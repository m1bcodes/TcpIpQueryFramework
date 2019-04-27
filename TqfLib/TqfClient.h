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
#include <boost/optional.hpp>

#include "TqfDecl.h"

using boost::asio::ip::tcp;

void send_buffer(tcp::socket& s, char* buffer, uint32_t length)
{
	TqfHeader header;
	header.command = 13;
	header.length = length;

	boost::asio::write(s, boost::asio::buffer(reinterpret_cast<const void*>(&header), sizeof(header)));
	boost::asio::write(s, boost::asio::buffer(buffer, length));
}


template <typename SyncReadStream, typename MutableBufferSequence, typename CompletionCondition>
void readWithTimeout(SyncReadStream& s, const MutableBufferSequence& buffers, const CompletionCondition& complCond, const boost::asio::deadline_timer::duration_type& expiry_time)
{
	boost::optional<boost::system::error_code> timer_result;
	boost::asio::deadline_timer timer(s.get_io_service());
	timer.expires_from_now(expiry_time);
	timer.async_wait([&timer_result](const boost::system::error_code & error) { timer_result.reset(error); });

	boost::optional<boost::system::error_code> read_result;
	boost::asio::async_read(s, buffers, complCond, [&read_result](const boost::system::error_code & error, size_t) { read_result.reset(error); });

	s.get_io_service().reset();
	while (s.get_io_service().run_one())
	{
		if (read_result)
			timer.cancel();
		else if (timer_result)
			s.cancel();
	}

	if (*read_result)
		throw boost::system::system_error(*read_result);
}

class TqfClient
{
public:

	TqfClient(const std::string& host, const std::string port)
		: resolver(io_service)
		, s(io_service)
	{
		tcp::resolver::query query(tcp::v4(), host, port);
		tcp::resolver::iterator iterator = resolver.resolve(query);
		s.connect(*iterator);
	}

	void send(int command, const std::vector<char>& argument)
	{
		TqfHeader header;
		header.command = command;
		header.length = argument.size();

		boost::asio::write(s, boost::asio::buffer(reinterpret_cast<const void*>(&header), sizeof(header)));
		boost::asio::write(s, boost::asio::buffer(argument));
	}

	void send(int command, const std::string& argument)
	{
		TqfHeader header;
		header.command = command;
		header.length = argument.size();

		boost::asio::write(s, boost::asio::buffer(reinterpret_cast<const void*>(&header), sizeof(header)));
		boost::asio::write(s, boost::asio::buffer(argument));
	}

	int read(std::vector<char>& data)
	{
		TqfHeader inHeader;

		auto timeout = boost::posix_time::seconds(10);
		readWithTimeout(boost::asio::buffer(&inHeader, sizeof(inHeader)), boost::asio::transfer_exactly(sizeof(inHeader)), timeout);

		std::cout << "Received header: cmd=" << inHeader.command << ", length=" << inHeader.length << "\n";

		data.resize(inHeader.length);
		readWithTimeout(boost::asio::buffer(data), boost::asio::transfer_exactly(inHeader.length), timeout);
		return inHeader.command;
	}

	tcp::socket& getSocket()
	{
		return s;
	}

protected:
	void send_buffer(int command, char* buffer, uint32_t length)
	{
		TqfHeader header;
		header.command = command;
		header.length = length;

		boost::asio::write(s, boost::asio::buffer(reinterpret_cast<const void*>(&header), sizeof(header)));
		boost::asio::write(s, boost::asio::buffer(buffer, length));
	}

	template <typename MutableBufferSequence, typename CompletionCondition>
	void readWithTimeout(const MutableBufferSequence& buffers, const CompletionCondition& complCond, const boost::asio::deadline_timer::duration_type& expiry_time)
	{
		boost::optional<boost::system::error_code> timer_result;
		boost::asio::deadline_timer timer(s.get_io_service());
		timer.expires_from_now(expiry_time);
		timer.async_wait([&timer_result](const boost::system::error_code & error) { timer_result.reset(error); });

		boost::optional<boost::system::error_code> read_result;
		boost::asio::async_read(s, buffers, complCond, [&read_result](const boost::system::error_code & error, size_t) { read_result.reset(error); });

		s.get_io_service().reset();
		while (s.get_io_service().run_one())
		{
			if (read_result)
				timer.cancel();
			else if (timer_result)
				s.cancel();
		}

		if (*read_result)
			throw boost::system::system_error(*read_result);
	}

private:
	boost::asio::io_service io_service;
	tcp::resolver resolver;
	tcp::socket s;
};
