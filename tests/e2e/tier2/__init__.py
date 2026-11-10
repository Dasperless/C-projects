"""
Tier 2: Boundary & Corner Cases Test Suite.
Imports all Tier 2 test modules to register test cases.
"""

from tests.e2e.tier2 import (
    test_t2_f01_build_limits,
    test_t2_f02_hygiene_boundary,
    test_t2_f03_ci_boundary,
    test_t2_f04_sll_edge_cases,
    test_t2_f05_sll_harness_limits,
    test_t2_f06_sll_algo_limits,
    test_t2_f07_vector_boundary,
    test_t2_f08_circular_buffer_wrap,
    test_t2_f09_dll_boundary,
    test_t2_f10_bst_hash_collision,
    test_t2_f11_chatroom_socket_limits,
    test_t2_f12_framing_corrupt_payload,
    test_t2_f13_logger_concurrency_stress,
    test_t2_f14_file_corrupt_overflow,
    test_t2_f15_admin_rapid_commands,
    test_t2_f16_style_boundary,
    test_t2_f17_docs_links_licenses,
)
