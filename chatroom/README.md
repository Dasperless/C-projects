Como vimos en clase, en esta Tarea haremos uso de conceptos básicos de comunicación entre procesos para crear una sencilla aplicación de chat en modo texto, a nivel de terminal, pero que nos permitiría incluso comunicar terminales en distintas computadoras que estén en una misma red.

Para esta Tarea se puede hacer uso del material de los capítulos 12 al 15 del libro: "Beginning Linux Programming, Fourth Edition.pdf"  que fueron asignados como Lecturas previas en este curso. El capítulo 11 también puede ser útil si se desea implementar señales como parte de la comunicación entre procesos.

Con estas herramientas vamos a proceder a implementar un sistema básico y sencillo de Chat desde la Terminal de Linux.

Todo sistema de Chat tiene dos componentes importantes: un servidor que tiene el control de los usuarios conectados al sistema, y un programa cliente usado en múltiples instancias por múltiples usuarios para intercambiar mensajes.

La interfaz será sencilla, usaremos las IP de las máquinas para identificar a los programas clientes del sistema y un nombre de usuario que será solicitado por el programa cliente al iniciar la sesión en el programa cliente ante el servidor.

Se debe definir un protocolo básico de comunicación entre el cliente y el servidor de forma tal que el cliente puede hacer consultas al servidor, tales como : cantidad de usuarios conectados, lista de nombres de usuarios conectados, iniciar conversación con un cliente, enviar mensaje de texto, enviar archivo, etc. Además el cliente podrá enviar un mensaje a un usuario particular indicando en la sesión cliente el nombre del usuario destinatario del mensaje y el mensaje o mensajes a enviar.. hasta que termine la sesión con dicho usuario y regrese al menú de opciones para hacer consultas al servidor o bien conversar con otro usuario en el sistema. 

Desde el programa cliente el usuario podrá crear una conversación grupal. Para comunicarse con varios usuarios en el sistema al mismo tiempo. Según lo comentado en clases, hay varias formas de implementar estos grupos, teniendo como referencia otras aplicaciones tales como Telegram, WhatsApp o Discord. 

Como desarrolladores deberán diseñar y definir varios aspectos del programa final. Desde la interfaz en modo texto, el protocolo de los mensajes y los mensajes en sí entre los procesos, la forma de conformación de los grupos y si la comunicación será toda centralizada vía el servidor o bien si será directa entre procesos. 

Deberá documentar todas estas definiciones y diseños realizados, debe brindar diagramas que expliquen estos funcionamientos y la forma de interacción y comunicación entre todas las partes. Deberá también mostrar las estructuras de datos desarrolladas y el código. 

Los programas pueden usar hilos (threads) o forks para poder llevar el control de las diversas actividades asíncronas: envío y recepción de mensajes, despliegues de opciones, etc. Recuerde usar semáforos para sincronizar los procesos y evitar inconssitencias de datos.

El programa servidor, además, tendrá un menú de opciones que permita listar los usuarios conectados (IP y nombre), así como mostrar el tráfico de mensajes en modo “monitor”. Debe mostrar información estadística de uso, como el número de clientes, cantidad de mensajes transmitido, cantidad de bytes transmitidos, etc.

El servidor deberá llevar un registro tipo LOG o bitácora de todas las actividades realizadas: llegada de usuarios, desconexión, mensajes transmitidos entre un par de usuarios, etc. Este archivo no debe borrarse al finalizar la sesión. El contenido nuevo se agragará siempre al final del archivo.

