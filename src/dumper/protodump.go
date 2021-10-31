package main

import (
	"encoding/base64"
	"io/ioutil"
	"os"
)

func main() {
	all, err := ioutil.ReadAll(os.Stdin)
	if err != nil {
		os.Exit(1)
	}
	_, _ = os.Stderr.WriteString("\n\n")
	_, _ = os.Stderr.WriteString(base64.RawStdEncoding.EncodeToString(all))
	_, _ = os.Stderr.WriteString("\n\n")

	// PATH=.:$PATH protoc     --proto_path . -I=. service1.proto  --protodump_out=./out 2> debugproto.b64
}
