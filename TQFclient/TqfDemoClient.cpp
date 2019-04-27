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

#include "..\TqfLib\TqfClient.h"

int main(int argc, char* argv[]) 
{
	try
	{
		if (argc != 3)
		{
			std::cerr << "Usage: blocking_tcp_echo_client <host> <port>\n";
			return 1;
		}

		TqfClient client(argv[1], argv[2]);
		const int max_length = 1024;
		char* request = new char[max_length];
		char* reply = new char[max_length];
		while (true) {
			using namespace std; // For strlen.
			std::cout << "Enter message: ";

			std::cin.getline(request, max_length);
			
			if (strcmp(request, "quit") == 0) {
				break;
			}
			else if (strcmp(request, "big") == 0) {
				for (int i = 0; i < max_length; i++)
					request[i] = '0' + (i % 10);
				request[max_length - 1] = '\0';
				client.send(13, request);
			}
			else {
				size_t request_length = strlen(request);
				client.send(13, request);
			}

			{
				std::vector<char> data;
				int retCode = client.read(data);
				std::string resp(data.begin(), data.end());
				std::cout << "return code=" << retCode << ", data=" << resp.substr(0, 100) << "\n";

			}
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
		system("pause");
	}

	return 0;
}