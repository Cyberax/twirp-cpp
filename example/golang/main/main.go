package main

import (
	"context"
	"example/example"
	"fmt"
	"net/http"
	"os"
)

func main() {
	client := example.NewHaberdasherProtobufClient("http://localhost:8080", &http.Client{})

	hat, err := client.MakeHat(context.Background(), &example.Size{Inches: 12})
	if err != nil {
		fmt.Printf("oh no: %v", err)
		os.Exit(1)
	}
	fmt.Printf("I have a nice new hat: %+v\n", hat)
}
