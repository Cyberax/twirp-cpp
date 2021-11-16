package main

import (
	"context"
	"example/example"
	"fmt"
	"github.com/twitchtv/twirp"
	"net/http"
	"os"
)

func main() {
	setHeaderHook := &twirp.ClientHooks{
		RequestPrepared: func(ctx context.Context, request *http.Request) (context.Context, error) {
			request.Header.Add("Authorization", "PrettyPlease")
			return ctx, nil
		}}
	client := example.NewHaberdasherProtobufClient("http://localhost:8080",
		http.DefaultClient, twirp.WithClientHooks(setHeaderHook))

	hat, err := client.MakeHat(context.Background(), &example.Size{Inches: 12})
	if err != nil {
		fmt.Printf("oh no: %v", err)
		os.Exit(1)
	}
	fmt.Printf("I have a nice new hat: %+v\n", hat)
}
