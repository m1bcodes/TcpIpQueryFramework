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

#pragma once

#define BOOST_ASIO_ENABLE_HANDLER_TRACKING
#include <boost/asio.hpp>

#include "TqfDecl.h"

using boost::asio::ip::tcp;

template<typename session>
using TqfSessionFactory = std::function<session(tcp::socket&&)>;

class TqfSession
	: public std::enable_shared_from_this<TqfSession>
{
public:
	TqfSession(tcp::socket socket)
		: socket_(std::move(socket))
	{
	}

	void start()
	{
		do_read_header();
	}

protected:

	virtual void processMessage(const TqfHeader& header, const std::vector<char>& payload)
	{
		std::string cmd(payload.begin(), payload.end());
		if (cmd == "ack") {
			sendResponse(retOK);
		}
		else if (cmd == "err") {
			sendResponse(retError, "Some error message was requested");
		}
		else if (cmd == "throw") {
			throw std::exception("Some error in the processMessage function");
		}
		else if (cmd == "hello") {
			sendResponse(header.command, "Hello client");
		}
		else if (cmd == "noresponse") {

		}
		else {
			sendResponse(header.command, payload);
		}
	}

	virtual void sendResponse(uint32_t returnCode, const std::vector<char>& payload = std::vector<char>())
	{
		outHeader.command = returnCode;
		outHeader.length = payload.size();
		outPayload = payload;
		do_write_header();
	}

	virtual void sendResponse(uint32_t returnCode, const std::string& message)
	{
		std::vector<char> v(message.begin(), message.end());
		sendResponse(returnCode, v);
	}

private:

	void do_read_header()
	{
		auto self(shared_from_this());
		socket_.async_read_some(boost::asio::buffer((void*)(&inHeader), 8),
			[this, self](boost::system::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					std::cout << "Received header: cmd=" << inHeader.command << ", length=" << inHeader.length << " bytes\n";
					inPayload.clear();
					inPayload.reserve(inHeader.length);
					do_read_payload();

				}
			});
	}
	void do_read_payload()
	{
		auto self(shared_from_this());
		socket_.async_read_some(boost::asio::buffer(data_, max_length),
			[this, self](boost::system::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					std::cout << "Received: " << length << " bytes\n";
					inPayload.insert(inPayload.end(), data_, data_ + length);
					if (inPayload.size() == inHeader.length) {
						std::cout << "Packet complete";

						try {
							outHeader.command = retNoReturn;
							processMessage(inHeader, inPayload);
							if (outHeader.command == retNoReturn) {
								// dont send return code, so wait for new incoming message
								do_read_header();
							}
						}
						catch (const std::exception & ex) {
							sendResponse(retError, ex.what());
						}
					}
					else if (inPayload.size() > inHeader.length) {
						std::cout << "Payload size mismatch";
					}
					else {
						do_read_payload();
					}
				}
			});
	}

	void do_write_header()
	{
		auto self(shared_from_this());
		std::cout << "writing header: resp=" << outHeader.command << ", length=" << outHeader.length << " bytes\n";
		boost::asio::async_write(socket_, boost::asio::buffer(reinterpret_cast<const void*>(&outHeader), sizeof(outHeader)),
			[this, self](boost::system::error_code ec, std::size_t /*length*/)
			{
				if (!ec)
				{
					do_write_payload();
				}
			});
	}
	void do_write_payload()
	{
		auto self(shared_from_this());

		std::cout << "writing: " << outPayload.size() << " bytes\n";
		boost::asio::async_write(socket_, boost::asio::buffer(outPayload),
			[this, self](boost::system::error_code ec, std::size_t /*length*/)
			{
				if (!ec)
				{

					do_read_header();
				}
			});
	}

	tcp::socket socket_;
	enum { max_length = 1024 };
	char data_[max_length];

	TqfHeader inHeader;
	std::vector<char> inPayload;
	TqfHeader outHeader;
	std::vector<char> outPayload;
};

template<typename session>
class TqfServer
{
public:
	TqfServer(boost::asio::io_context& io_context, short port, TqfSessionFactory<session> factory)
		: acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
		, factory_(factory)
	{
		do_accept();
	}

private:
	void do_accept()
	{
		acceptor_.async_accept(
			[this](boost::system::error_code ec, tcp::socket socket)
			{
				std::cout << "Connection from " << socket.remote_endpoint().address().to_string() << "\n";
				if (!ec)
				{
					// session s = factory();
					// std::make_shared<session>(std::move(socket))->start();
					//std::shared_ptr<session>(new session(std::move(socket), 42))->start();
				
					std::make_shared<session>(factory_(std::move(socket)))->start();
				}

				do_accept();
			});
	}

	tcp::acceptor acceptor_;
	TqfSessionFactory<session> factory_;
};


template<typename session>
void TqfRunServer(int port)
{
	boost::asio::io_context io_context;

	TqfServer<session> s(io_context, port, [](tcp::socket && s) { return session(std::move(s)); });

	io_context.run();
}

template<typename session>
void TqfRunServer(int port, TqfSessionFactory<session> factory)
{
	boost::asio::io_context io_context;

	TqfServer<session> s(io_context, port, factory);

	io_context.run();
}

