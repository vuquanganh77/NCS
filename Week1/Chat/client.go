package main

import (
	"bufio"
	"fmt"
	"net"
	"os"
	// "strings"
)

func main() {
	conn, err := net.Dial("tcp", "localhost:8080")
	if err != nil {
		fmt.Println("Error connecting to server:", err.Error())
		return
	}
	defer conn.Close()

	fmt.Print("Enter your name: ")
	clientName := readInput()

	// Send the name of the client to the server
	conn.Write([]byte(clientName + "\n"))

	// Doc va hien thi danh sach client dang online tu server
	initialMessage := readData(conn)
	fmt.Println(initialMessage)

	// Hien thi cac tin nhan tu server
	go func() {
		for {
			message := readData(conn)
			fmt.Print(message)
		}
	}()

	// Continuously handle user input
	handleInput(conn)
}

func handleInput(conn net.Conn) {
	scanner := bufio.NewScanner(os.Stdin)

	fmt.Print("Enter message (type '/select [client]' to start private chat, type 'exit' to quit): ")
	for {
		scanner.Scan()
		message := scanner.Text()

		// Gui tin nhan len server
		conn.Write([]byte(message + "\n"))

		// if strings.ToLower(message) == "exit" {
		// 	return
		// }
	}
}

func readData(conn net.Conn) string {
	buffer := make([]byte, 1024)
	n, err := conn.Read(buffer)
	if err != nil {
		fmt.Println("Error reading:", err.Error())
		os.Exit(0)
	}
	return string(buffer[:n])
}

func readInput() string {
	scanner := bufio.NewScanner(os.Stdin)
	scanner.Scan()
	return scanner.Text()
}
