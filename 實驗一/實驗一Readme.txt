實驗一
1. Layer2 Communication
- 目錄: 實驗一/Layer2 Communication/1NIC/
- 1.1.1 client
- 目錄: client/layer_tcp/
- 執行環境: Linux
- 指令步驟: 
1) make (編譯出執行檔receiver)
2) ./receiver <sender_hw_addr> <sender_ip> <run_times>
(p.s.<sender_hw_addr>: 對方的MAC Address; <sender_ip>: 對方的IP; <run_times>: 送的封包數)
- 1.1.2 server
- 目錄: server/layer_tcp/
- 執行環境: Linux
- 指令步驟: 
1) make (編譯出執行檔sender)
2) ./sender <run_times>
(p.s. <run_times>: 送的封包數)

2. TCP
- 目錄: 實驗一/TCP/
- 2.1.1 client
- 目錄: client/memcpy/test_tcp
- 執行環境: Linux
- 指令步驟: 
1) make (編譯出執行檔client)
2) ./client <server_ip>
- 2.1.2 server
- 目錄: server/memcpy/test_tcp
- 執行環境: Linux
- 指令步驟: 
1) make (編譯出執行檔server)
2) ./server <num_packets>
