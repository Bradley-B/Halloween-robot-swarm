package main

import (
	"context"
	_ "embed"
	"fmt"
	"log"
	"net"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	natsserver "github.com/nats-io/nats-server/v2/server"
)

//go:embed index.html
var indexHTML []byte

func main() {
	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()
	if err := run(ctx); err != nil {
		log.Fatal(err)
	}
}

func run(ctx context.Context) error {
	nats, err := startNATS(4223)
	if err != nil {
		return err
	}
	defer func() {
		nats.Shutdown()
		nats.WaitForShutdown()
	}()
	log.Printf("NATS server running at %s", nats.ClientURL())

	mux := http.NewServeMux()
	mux.HandleFunc("GET /", func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path != "/" {
			http.NotFound(w, r)
			return
		}
		w.Header().Set("Content-Type", "text/html; charset=utf-8")
		w.Write(indexHTML)
	})

	server := &http.Server{
		Addr:              "localhost:8095",
		Handler:           mux,
		ReadHeaderTimeout: 5 * time.Second,
	}

	listener, err := net.Listen("tcp", server.Addr)
	if err != nil {
		return fmt.Errorf("listen for HTTP: %w", err)
	}
	defer server.Close()
	errorsCh := make(chan error, 1)
	go func() {
		errorsCh <- server.Serve(listener)
	}()
	log.Println("HTTP server running at http://localhost:8095")

	select {
	case err := <-errorsCh:
		return fmt.Errorf("serve HTTP: %w", err)
	case <-ctx.Done():
		log.Println("Shutting down HTTP and NATS servers")
		shutdownCtx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
		defer cancel()
		if err := server.Shutdown(shutdownCtx); err != nil {
			return fmt.Errorf("shut down HTTP: %w", err)
		}
		return nil
	}
}

func startNATS(port int) (*natsserver.Server, error) {
	nats, err := natsserver.NewServer(&natsserver.Options{
		ServerName: "halloween-robot-swarm",
		Host:       "127.0.0.1",
		Port:       port,
		NoSigs:     true,
	})
	if err != nil {
		return nil, fmt.Errorf("create NATS server: %w", err)
	}
	go nats.Start()
	if !nats.ReadyForConnections(5 * time.Second) {
		nats.Shutdown()
		nats.WaitForShutdown()
		return nil, fmt.Errorf("NATS server was not ready within 5 seconds; check whether port %d is available", port)
	}
	return nats, nil
}
