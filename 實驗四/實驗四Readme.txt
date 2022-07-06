實驗四
- 1.1.1 client
- 目錄: 實驗四/client/udt4
- 執行環境: Linux
- 指令步驟: 
1) 進入udt4/src/
2) make (編譯出libudt.so)
3) 設定libudt.so路徑(設定LD_LIBRARY_PATH)
4) 進入udt4/app/
5) make (編譯出執行檔udt_tcp_client)
6) ./udtclient <server_ip> <server_port> <output_interval(sec)> <mode(1 or 2)> <tcp_port>
(p.s.<server_ip>: server的IP; <server_port>: server的port; <output_interval(sec)>: 多久印一次資料, 單位是sec; <mode(1 or 2)>: mode1=>cumulative mode,印出的結果為累積後的結果; mode2=>interval mode, 印出的結果為每個interval的結果; <tcp_port>: tcp server的port)
- 1.1.2 server
- 目錄: 實驗四/server/udt4/
- 執行環境: Linux
- 指令步驟: 
1) 進入udt4/src/
2) make (編譯出libudt.so)
3) 設定libudt.so路徑(設定LD_LIBRARY_PATH)
4) 進入udt4/app/
5) make (編譯出執行檔udt_tcp_server)
6) ./udt_tcp_server <udt_control_port> <execute_time(sec)> <num_client> <output_interval(sec)> <ttl(msec)> <tcp_port> <udt_data_port>
(p.s. <udt_control_port> : udt server用來交換control的port; <execute_time(sec)>: 程式執行的時間;  <num_client> : client的個數; <output_interval(sec)>: 多久印一次資料;  <ttl(msec)>: 設定ttl, 單位是msec;  <tcp_port>: tcp server的port; <udt_data_port>: udt server用來交換data的port)
