# TCPIP Query Framework

This is a boost asio based, header only library, which makes it easy to implment server and client pairs, which communicate via TCP/IP. The client sends a packet consisting of an integer command identifier and some arbitrary length payload to the server. The server responds with a packet of the same format: an integer return code and some arbitrary length data payload.

## Implementing the server

Just create your own derived class from `TqfSession` and override the funtion `processMessage`. The following example also implements an implementation specific argument `argument` and demonstrates how to propagate it from the initialization of the server to the creation of the session:

```
class MyTqfSession : public TqfSession
{
public:
	MyTqfSession(tcp::socket socket, int argument)
		: TqfSession(std::move(socket))
		, m_arg(argument)
	{}

protected:
	virtual void processMessage(const TqfHeader& header, const std::vector<char>& payload) override
	{
		std::string cmd(payload.begin(), payload.end());
		std::cout << "Received: cmd=" << header.command << ", cmd=" << cmd << "\n";
		if (cmd == "mycmd") {
			sendResponse(retOK, "Hello from mytqf");
		}
		else
			TqfSession::processMessage(header, payload);
	}

private:
	int m_arg;
};
```

The server is started with the following command:
```
TqfRunServer<MyTqfSession>(port, [](tcp::socket&& s) { return MyTqfSession(std::move(s), 42); });
```` 
If the derived TqfSession class does not require additional constructor arguments, a simpler version of TqfRunServer can be used:
```
TqfRunServer<MyTqfSession>(port);
```

## Implementing the client
This is as easy as it can get. 
1. Create the client and provide the remote host address and the port number:

```
TqfClient client(host, port);
```

2. send a command with optional argument to the server
```
client.send(command_id, request);
```

3. and receive the response data:
```
std::vector<char> data;
int retCode = client.read(data);
```

