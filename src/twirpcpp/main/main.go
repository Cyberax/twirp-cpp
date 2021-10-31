package main

import (
	"bytes"
	"encoding/base64"
	"github.com/Cyberax/twirpcpp/twirpcpp"
	pgs "github.com/lyft/protoc-gen-star"
	"io"
	"io/ioutil"
	"os"
)

func main() {
	src := os.Getenv("PROTO_SOURCE")
	var dataReader io.Reader = os.Stdin
	if src != "" {
		in, err := os.Open(src)
		if err != nil {
			println(err.Error())
			os.Exit(1)
		}
		data, err := ioutil.ReadAll(in)
		if err != nil {
			println(err.Error())
			os.Exit(1)
		}
		decoded, err := base64.RawStdEncoding.DecodeString(string(data))
		if err != nil {
			println(err.Error())
			os.Exit(1)
		}
		dataReader = bytes.NewReader(decoded)
	}
	pgs.Init(pgs.DebugEnv("DEBUG_PGV"), pgs.ProtocInput(dataReader)).
		RegisterModule(twirpcpp.Validator()).
		Render()
}
