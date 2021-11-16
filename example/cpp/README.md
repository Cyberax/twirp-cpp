# Example Project

This is an example C++ project for the Twirp protocol. It contains both a C++ server and client. 

The `conanfile.py` can be replaced with a `conanfile.txt` for actual end-user projects. The Python
version is used only to automatically pick up the current version for CI/CD reasons.

## Compiling 
Once you install `twirp-cpp` (see the `twirp-cpp` README.md for details), simply use regular CMake:

```bash
mkdir build
cd build
cmake ..
make -j8
```

This will produce `build/bin/webserver` and `build/bin/webclient`. 

## Running

The `webserver` binary runs a server on `localhost:8080` and the `weblcient` connects to it. Running
them is straightforward:

```bash
$ ./build/bin/webserver
Request received: /twirp/twitch.twirp.example.Haberdasher/MakeHat, resp=401
Request received: /twirp/twitch.twirp.example.Haberdasher/MakeHat, resp=200
Request received: /twirp/twitch.twirp.example.Haberdasher/MakeHat, resp=400
```

```bash
$ ./build/bin/webclient
Yay! Got an unauthenticated response!
name = Magic Hat
color = beige
size = 32
Got expected result!
```

## Running Go code

You can also test the multi-language interoperability by running a Go-based client. Simply navigate to
`example/golang` and run `go run main/main.go`. It will compile and run the client that connects to 
the server running on 'localhost:8080':

```bash
$ go run main/main.go
I have a nice new hat: size:12  color:"beige"  name:"Magic Hat"
```

## Implementation notes

The server and client are both fairly prototypical for a simple Twirp-based web service. They both use the 
`httplib`-based server and client. The server implements a very simple request authentication, by
checking the `Authorization` header that must be equal to `PrettyPlease`.

This is done in the `AuthenticationMiddleware` class. The middleware then stores the `Principal` object
in the request context in case of success. The client code retrieves this object or gets the default value
(specified in `AuthenticationData::Default()`), the default value has `is_anonymous_` flag set.

There's a simple demonstration of error handling, for an unauthenticated request (thrown by the middleware) 
and the application-specific `OutOfRangeError` if the hat size is too large.
