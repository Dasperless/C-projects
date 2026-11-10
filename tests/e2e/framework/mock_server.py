"""
High-fidelity reference chatroom server implementing Interface Contract 3.
Provides deterministic, opaque-box network socket testing for protocol framing,
concurrency, private messaging, groups, file transfer, and admin commands.
"""

import os
import socket
import threading
import time
from typing import Dict, Set, Optional, Tuple, List
from tests.e2e.framework.protocol import (
    PacketHeader,
    PacketType,
    compute_crc32,
    HEADER_SIZE_STANDARD,
    HEADER_SIZE_PACKED,
)


class MockChatServer:
    def __init__(self, host: str = "127.0.0.1", port: int = 0, log_path: Optional[str] = None):
        self.host = host
        self.port = port
        self.log_path = log_path
        self.server_sock: Optional[socket.socket] = None
        self.running = False
        self.thread: Optional[threading.Thread] = None

        # State
        self.clients: Dict[str, socket.socket] = {}  # username -> socket
        self.groups: Dict[str, Set[str]] = {}  # group_name -> set(usernames)
        self.active_transfers: Dict[str, Dict] = {}  # transfer_id -> info
        self.stats = {"messages": 0, "transfers": 0, "kicks": 0}
        self.lock = threading.Lock()
        self.log_lock = threading.Lock()

    def start(self) -> int:
        self.server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_sock.bind((self.host, self.port))
        self.server_sock.listen(50)
        self.port = self.server_sock.getsockname()[1]
        self.running = True

        self.thread = threading.Thread(target=self._accept_loop, daemon=True)
        self.client_threads: List[threading.Thread] = []
        self.thread.start()
        self._log("SERVER_START", f"Server listening on port {self.port}")
        return self.port

    def stop(self) -> None:
        self.running = False
        if self.server_sock:
            try:
                self.server_sock.close()
            except Exception:
                pass
        with self.lock:
            for sock in list(self.clients.values()):
                try:
                    sock.close()
                except Exception:
                    pass
            self.clients.clear()
        for t in list(self.client_threads):
            try:
                t.join(timeout=0.05)
            except Exception:
                pass
        self._log("SERVER_STOP", "Server shutdown gracefully")

    def _log(self, event: str, message: str) -> None:
        if not self.log_path:
            return
        ts = time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime())
        line = f"[{ts}] [{event}] {message}\n"
        try:
            with self.log_lock:
                os.makedirs(os.path.dirname(os.path.abspath(self.log_path)), exist_ok=True)
                with open(self.log_path, "a", encoding="utf-8") as f:
                    f.write(line)
        except Exception:
            pass

    def _accept_loop(self) -> None:
        while self.running:
            try:
                client_sock, addr = self.server_sock.accept()
                t = threading.Thread(target=self._handle_client, args=(client_sock, addr), daemon=True)
                self.client_threads.append(t)
                t.start()
            except Exception:
                break

    def _handle_client(self, sock: socket.socket, addr: Tuple[str, int]) -> None:
        username = ""
        try:
            while self.running:
                hdr_bytes = self._recv_exact(sock, HEADER_SIZE_STANDARD)
                if not hdr_bytes:
                    break
                header = PacketHeader.unpack(hdr_bytes, packed=False)
                payload = self._recv_exact(sock, header.payload_len) if header.payload_len > 0 else b""

                if header.type == PacketType.PKT_CONNECT:
                    username = header.sender.strip()
                    with self.lock:
                        if username in self.clients:
                            # Username collision
                            err_pkt = PacketHeader(PacketType.PKT_ERROR, 16, "server", username).pack() + b"Name already in use"
                            sock.sendall(err_pkt)
                            break
                        self.clients[username] = sock
                    self._log("CONNECT", f"User {username} connected from {addr[0]}:{addr[1]}")

                elif header.type == PacketType.PKT_DISCONNECT:
                    self._log("DISCONNECT", f"User {username} disconnected voluntarily")
                    break

                elif header.type == PacketType.PKT_PUBLIC_MSG:
                    self._broadcast(username, payload)
                    self.stats["messages"] += 1
                    self._log("RELAY", f"Public msg from {username}: {len(payload)} bytes")

                elif header.type == PacketType.PKT_PRIVATE_MSG:
                    target_user = header.target.strip()
                    with self.lock:
                        target_sock = self.clients.get(target_user)
                    if target_sock:
                        pkt = PacketHeader(PacketType.PKT_PRIVATE_MSG, len(payload), username, target_user).pack() + payload
                        target_sock.sendall(pkt)
                        self.stats["messages"] += 1
                        self._log("RELAY", f"Private msg from {username} to {target_user}")
                    else:
                        err = f"User '{target_user}' not found".encode("utf-8")
                        err_pkt = PacketHeader(PacketType.PKT_ERROR, len(err), "server", username).pack() + err
                        sock.sendall(err_pkt)

                elif header.type == PacketType.PKT_GROUP_MSG:
                    group_name = header.target.strip()
                    self._send_group(group_name, username, payload)
                    self.stats["messages"] += 1
                    self._log("RELAY", f"Group msg to [{group_name}] from {username}")

                elif header.type == PacketType.PKT_USER_LIST_REQ:
                    with self.lock:
                        user_list = ",".join(self.clients.keys()).encode("utf-8")
                    resp = PacketHeader(PacketType.PKT_USER_LIST_RESP, len(user_list), "server", username).pack() + user_list
                    sock.sendall(resp)

                elif header.type == PacketType.PKT_FILE_OFFER:
                    # Target is receiver; payload contains filename:size:crc32
                    target_user = header.target.strip()
                    with self.lock:
                        target_sock = self.clients.get(target_user)
                    if target_sock:
                        pkt = PacketHeader(PacketType.PKT_FILE_OFFER, len(payload), username, target_user).pack() + payload
                        target_sock.sendall(pkt)
                        self._log("FILE_OFFER", f"File offer from {username} to {target_user}")

                elif header.type == PacketType.PKT_FILE_CHUNK:
                    target_user = header.target.strip()
                    with self.lock:
                        target_sock = self.clients.get(target_user)
                    if target_sock:
                        pkt = PacketHeader(PacketType.PKT_FILE_CHUNK, len(payload), username, target_user).pack() + payload
                        target_sock.sendall(pkt)
                        self.stats["transfers"] += 1

                elif header.type == PacketType.PKT_ADMIN_CMD:
                    cmd_str = payload.decode("utf-8", errors="replace").strip()
                    resp_str = self._handle_admin_cmd(cmd_str)
                    resp_bytes = resp_str.encode("utf-8")
                    pkt = PacketHeader(PacketType.PKT_ADMIN_RESP, len(resp_bytes), "server", username).pack() + resp_bytes
                    sock.sendall(pkt)

        except Exception:
            pass
        finally:
            if username:
                with self.lock:
                    if username in self.clients:
                        del self.clients[username]
                    for gname, members in list(self.groups.items()):
                        members.discard(username)
                self._log("DISCONNECT", f"Connection cleaned up for {username}")
            try:
                sock.close()
            except Exception:
                pass

    def _broadcast(self, sender: str, payload: bytes) -> None:
        pkt = PacketHeader(PacketType.PKT_PUBLIC_MSG, len(payload), sender, "").pack() + payload
        with self.lock:
            for uname, csock in list(self.clients.items()):
                if uname != sender:
                    try:
                        csock.sendall(pkt)
                    except Exception:
                        pass

    def _send_group(self, group: str, sender: str, payload: bytes) -> None:
        with self.lock:
            if group not in self.groups:
                self.groups[group] = set()
            self.groups[group].add(sender)
            members = list(self.groups[group])

        pkt = PacketHeader(PacketType.PKT_GROUP_MSG, len(payload), sender, group).pack() + payload
        with self.lock:
            for member in members:
                if member != sender and member in self.clients:
                    try:
                        self.clients[member].sendall(pkt)
                    except Exception:
                        pass

    def _handle_admin_cmd(self, cmd: str) -> str:
        parts = cmd.split()
        if not parts:
            return "ERROR: Empty command"
        op = parts[0].lower()
        if op == "users":
            with self.lock:
                return f"ACTIVE_USERS: {','.join(self.clients.keys())}"
        elif op == "stats":
            with self.lock:
                return f"STATS: clients={len(self.clients)} messages={self.stats['messages']} transfers={self.stats['transfers']} kicks={self.stats['kicks']}"
        elif op == "kick" and len(parts) > 1:
            target = parts[1]
            with self.lock:
                target_sock = self.clients.get(target)
            if target_sock:
                try:
                    target_sock.close()
                except Exception:
                    pass
                with self.lock:
                    if target in self.clients:
                        del self.clients[target]
                self.stats["kicks"] += 1
                self._log("ADMIN", f"Admin kicked user {target}")
                return f"OK: Kicked {target}"
            return f"ERROR: User {target} not found"
        return f"UNKNOWN_COMMAND: {op}"

    def _recv_exact(self, sock: socket.socket, length: int) -> bytes:
        data = bytearray()
        while len(data) < length:
            chunk = sock.recv(length - len(data))
            if not chunk:
                return bytes(data)
            data.extend(chunk)
        return bytes(data)
