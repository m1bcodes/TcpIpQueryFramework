//MIT License

//Copyright(c) 2019 mibcoder

//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this softwareand associated documentation files(the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions :

//The above copyright noticeand this permission notice shall be included in all
//copies or substantial portions of the Software.

//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

#include <iostream>

#include "..\TqfLib\TqfServer.h"

class MyTqfSession : public TqfSession
{
public:
	MyTqfSession(tcp::socket socket, int argument)
		: TqfSession(std::move(socket))
		, m_arg(argument)
	{

	}

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


int main(int argc, char* argv[])
{
	try
	{
		if (argc != 2)
		{
			std::cerr << "Usage: TqfServer <port>\n";
			return 1;
		}
		int port = std::atoi(argv[1]);

		TqfRunServer<MyTqfSession>(port, [](tcp::socket&& s) { return MyTqfSession(std::move(s), 42); });
	}
	catch (std::exception & e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}