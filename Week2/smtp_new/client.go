package main

import (
	"bufio"
	"fmt"
	"net"
	"os"
	"regexp"
	// "strings"
)

func isValidEmail(email string) bool {
	// Use a regular expression to check for a valid email format
	// This is a simple example and may not cover all edge cases
	// You might want to use a more comprehensive email validation regex
	// or a dedicated email validation library.
	emailRegex := regexp.MustCompile(`^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$`)
	return emailRegex.MatchString(email)
}

func main() {
	conn, err := net.Dial("tcp", "localhost:8080")
	if err != nil {
		fmt.Println("Error connecting to server:", err.Error())
		return
	}
	defer conn.Close()
	var clientName string

	for {
		fmt.Print("Enter your email: ")
		clientName = readInput()

		if isValidEmail(clientName) {
			break
		} else {
			fmt.Println("Invalid email format. Please enter a valid email.")
		}
	}

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

	fmt.Print("Enter message (type '/select [email]' to start sending mail, type 'exit' to quit): ")
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
