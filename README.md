# stream-deal
#处理流黏包问题，采用unix domain socket 进行模拟，client端发包，服务端接收包，并做解包处理
消息采用0x1a2b3c4d 做包开始分割符，每个包的长度随机，每次接收的最大包长为8096，然后再内存中进行黏包分割处理，不够一个包放在leftbuf，留到下一次处理
