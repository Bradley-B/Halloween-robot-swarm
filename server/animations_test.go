package main

import (
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"testing"
)

func TestCustomAnimationCatalog(t *testing.T) {
	previous := customAnimationsJSON
	t.Cleanup(func() { customAnimationsJSON = previous })
	customAnimationsJSON = []byte(`[{"name":"spooky","part":"eyes","frames":[[224]],"durations":[125]}]`)
	response := httptest.NewRecorder()
	newHandler(nil).ServeHTTP(response, httptest.NewRequest(http.MethodGet, "/api/animations", nil))
	if response.Code != http.StatusOK {
		t.Fatalf("catalog status = %d", response.Code)
	}
	var options []map[string]any
	if err := json.Unmarshal(response.Body.Bytes(), &options); err != nil {
		t.Fatal(err)
	}
	if len(options) != 1 || options[0]["name"] != "spooky" || options[0]["part"] != "eyes" || len(options[0]) != 2 {
		t.Fatalf("unexpected catalog: %s", response.Body.String())
	}
}
