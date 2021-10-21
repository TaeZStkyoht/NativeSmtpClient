# NativeSmtpClient
A simple SMTP client for modern C++. It's native; since, it hasn't any dependency to Boost, cURL etc. It simply uses std::asio.

# Installation
```
mkdir build
cd build
cmake [-DBUILD_EXAMPLES=NO] ..
make install
```
If you are on Linux you may also need `ldconfig /usr/local/lib/`
