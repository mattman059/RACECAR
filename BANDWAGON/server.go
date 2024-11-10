package main

import (
    "crypto/rand"
    "encoding/hex"
    "fmt"
    "net"
    "sync"
)

var (
    // Mutex and map for storing UUID records in a thread-safe way
    uuidStore = make(map[string]struct{})
    mu        sync.Mutex
)

// generateUUID generates a 16-character UUID in the format xxxx-xxxx-xxxx-xxxx.
func generateUUID() (string, error) {
    b := make([]byte, 8) // 8 bytes = 16 hex characters
    _, err := rand.Read(b)
    if err != nil {
        return "", err
    }
    uuid := fmt.Sprintf("%s-%s-%s-%s", hex.EncodeToString(b[:2]), hex.EncodeToString(b[2:4]), hex.EncodeToString(b[4:6]), hex.EncodeToString(b[6:]))
    return uuid, nil
}

// handleConnection handles incoming connections, generates a UUID, and stores it.
func handleConnection(conn net.Conn) {
    defer conn.Close()

    uuid, err := generateUUID()
    if err != nil {
        fmt.Println("Error generating UUID:", err)
        return
    }

    // Store UUID in a thread-safe way
    mu.Lock()
    uuidStore[uuid] = struct{}{}
    mu.Unlock()

    // Send the UUID back to the client
    conn.Write([]byte(uuid + "\n"))
    fmt.Println("Stored UUID:", uuid)
}

func main() {
    listener, err := net.Listen("tcp", ":12345") // Bind to port 12345
    if err != nil {
        fmt.Println("Error starting TCP server:", err)
        return
    }
    defer listener.Close()
    fmt.Println("TCP server listening on port 12345...")

    for {
        conn, err := listener.Accept()
        clientAddr := conn.RemoteAddr().String()
        fmt.Sprintf("%s",clientAddr)
        if err != nil {
            fmt.Println("Error accepting connection:", err)
            continue
        }

        // Handle each connection concurrently
        go handleConnection(conn)
    }
}
