package main

import (
	"fmt"
	"net"
	"strings"
	"sync"
	"os"
	"bufio"
)

var (
	clients = make(map[string]net.Conn)
	clientsMux sync.Mutex
)

func sendOnlineClients(conn net.Conn) {
	clientsMux.Lock()
	defer clientsMux.Unlock()

	var onlineClients []string
	for client := range clients {
		onlineClients = append(onlineClients, client)
	}

	clientList := strings.Join(onlineClients, ", ")
	conn.Write([]byte("Invalid mail lists: " + clientList + "\n"))
}

func main() {
	listener, err := net.Listen("tcp", ":8080")
	if err != nil {
		fmt.Println("Error listening:", err.Error())
		return
	}
	defer listener.Close()

	fmt.Println("Server is listening on port 8080")

	// accept connection cua client
	for {
		conn, err := listener.Accept()
		if err != nil {
			fmt.Println("Error accepting connection:", err.Error())
			return
		}

		go handleConnection(conn)
	}
}

func handleConnection(conn net.Conn) {
	defer conn.Close()

	fmt.Println("Client connected:", conn.RemoteAddr())

	// Lay ten cua client
	clientName := readData(conn)

	clientsMux.Lock()
	clients[clientName] = conn
	clientsMux.Unlock()

	// Gui danh sach cac client den client moi nhat tham gia
	sendOnlineClients(conn)

	// Kiem tra xem tin nhan la exit hay chon 1 client
	for {
		message := readData(conn)
		if strings.ToLower(message) == "exit" {
			clientsMux.Lock()
			delete(clients, clientName)
			clientsMux.Unlock()
			break
		}

		// Xu ly neu tin nhan la gui toi 1 client
		if strings.HasPrefix(message, "/select ") {
			selectedClient := strings.TrimPrefix(message, "/select ")
			handleSelectClient(clientName, selectedClient, conn)
		} else {
			handleMessage(clientName, message)
			fmt.Println("Unexpected error, type another message")
		}
	}
}

func handleSelectClient(sender, selectedClient string, conn net.Conn) {
	clientsMux.Lock()
	defer clientsMux.Unlock()

	// Check client co ton tai hay khong
	selectedConn, ok := clients[selectedClient]
	if !ok {
		conn.Write([]byte("Client " + selectedClient + " is not online.\n"))
		return
	}

	// Thong bao cho 2 client
	//conn.Write([]byte("Starting private chat with " + selectedClient + "\n"))
	//selectedConn.Write([]byte("Starting private chat with " + sender + "\n"))

	// Use WaitGroup to synchronize goroutines
	var wg sync.WaitGroup
	wg.Add(2)
	var messages []string
	// Trao doi tin nhan giua 2 client
	go func() {
		defer wg.Done()
		for {
			message := readData(conn)
			if strings.ToLower(message) == "/end" {
				//conn.Write([]byte("Ending private chat with " + selectedClient + "\n"))
				//selectedConn.Write([]byte("Ending private chat with " + sender + "\n"))
				return
			}
			if strings.ToLower(message) == "/send"{

				for _, msg := range messages {
					selectedConn.Write([]byte(msg + "\n"))
				}
				// Clear the slice after sending
				messages = nil
			} else {
				// Store the message in the slice
				messages = append(messages, message)
			}
		}
	}()

	go func() {
		defer wg.Done()
		for {
			message := readData(selectedConn)
			if strings.ToLower(message) == "/end" {
				//selectedConn.Write([]byte("Ending private chat with " + sender + "\n"))
				//conn.Write([]byte("Ending private chat with " + selectedClient + "\n"))
				return
			}
			if strings.ToLower(message) == "/send"{

				for _, msg := range messages {
					conn.Write([]byte(msg + "\n"))
				}
				// Clear the slice after sending
				messages = nil
			} else {
				// Store the message in the slice
				messages = append(messages, message)
			}
		}
	}()

	// Wait 2 routines hoan thanh
	wg.Wait()
}

func handleMessage(sender string, message string) {
	clientsMux.Lock()
	defer clientsMux.Unlock()

	// ghep ten nguoi gui va tin nhan nguoi gui theo format a: abc
	parts := strings.Split(message, ":")
	if len(parts) < 2 {
		return
	}

	recipient := strings.TrimSpace(parts[0])
	messageBody := strings.Join(parts[1:], ":")

	recipientConn, ok := clients[recipient]
	if !ok {
		return
	}

	recipientConn.Write([]byte( messageBody + "\n"))
}


func readData(conn net.Conn) string {
    // Using bufio.Reader to read the complete line
    reader := bufio.NewReader(conn)
    message, err := reader.ReadString('\n')   
    if err != nil {
        fmt.Println("Error reading:", err.Error())
        os.Exit(0)
    }
    return strings.TrimSpace(message)
}

