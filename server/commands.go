package main

import (
	"encoding/json"
	"errors"
	"io"
	"log"
	"mime"
	"net/http"
	"strings"
	"time"
	"unicode"

	natsserver "github.com/nats-io/nats-server/v2/server"
	"github.com/nats-io/nats.go"
)

type commandRequest struct {
	Subject string          `json:"subject"`
	Message json.RawMessage `json:"message"`
}

func newHandler(publisher *nats.Conn) http.Handler {
	mux := http.NewServeMux()
	mux.HandleFunc("GET /{$}", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "text/html; charset=utf-8")
		w.Write(indexHTML)
	})
	mux.HandleFunc("POST /api/commands", func(w http.ResponseWriter, r *http.Request) {
		mediaType, _, err := mime.ParseMediaType(r.Header.Get("Content-Type"))
		if err != nil || mediaType != "application/json" {
			writeJSON(w, http.StatusUnsupportedMediaType, map[string]string{"error": "Send application/json."})
			return
		}

		r.Body = http.MaxBytesReader(w, r.Body, 64*1024)
		decoder := json.NewDecoder(r.Body)
		decoder.DisallowUnknownFields()
		var command commandRequest
		err = decoder.Decode(&command)
		if err == nil {
			var extra any
			if trailingErr := decoder.Decode(&extra); trailingErr != io.EOF {
				if trailingErr == nil {
					err = errors.New("multiple JSON values")
				} else {
					err = trailingErr
				}
			}
		}
		if err != nil {
			var tooLarge *http.MaxBytesError
			if errors.As(err, &tooLarge) {
				writeJSON(w, http.StatusRequestEntityTooLarge, map[string]string{"error": "Command exceeds the 64 KiB request limit."})
			} else {
				writeJSON(w, http.StatusBadRequest, map[string]string{"error": "Send one JSON object with subject and message fields."})
			}
			return
		}
		if !validPublishSubject(command.Subject) {
			writeJSON(w, http.StatusBadRequest, map[string]string{"error": "Subject must have nonempty dot-separated tokens, without whitespace, control characters, or wildcards."})
			return
		}
		if len(command.Message) == 0 || string(command.Message) == "null" {
			writeJSON(w, http.StatusBadRequest, map[string]string{"error": "A non-null JSON message is required."})
			return
		}

		err = publisher.Publish(command.Subject, command.Message)
		if err == nil {
			err = publisher.FlushTimeout(2 * time.Second)
		}
		if err != nil {
			log.Printf("Publish command to %q: %v", command.Subject, err)
			writeJSON(w, http.StatusServiceUnavailable, map[string]string{"error": "Could not confirm publishing to NATS. Check the server before retrying."})
			return
		}
		writeJSON(w, http.StatusOK, map[string]string{"status": "published", "subject": command.Subject})
	})
	return mux
}

func validPublishSubject(subject string) bool {
	return natsserver.IsValidLiteralSubject(subject) &&
		!strings.ContainsAny(subject, "*>") &&
		strings.IndexFunc(subject, func(r rune) bool { return unicode.IsSpace(r) || unicode.IsControl(r) }) == -1
}

func writeJSON(w http.ResponseWriter, status int, body any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	json.NewEncoder(w).Encode(body)
}
