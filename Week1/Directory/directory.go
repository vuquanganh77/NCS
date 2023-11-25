package main 

import(
	"fmt"
	"bufio"
	"io/ioutil"
	"os"
	"path/filepath"
)

func listFiles(dir string, depth int, results *[]string){
	if depth > 10 {
		return
	}

	files,err := ioutil.ReadDir(dir) // doc cac file ben trong dir
	if err != nil {
		fmt.Println("Loi doc file trong thu muc")
		return
	}

	for _,file := range files {
		if file.IsDir() {
			subdir := filepath.Join(dir,file.Name())
			listFiles(subdir, depth+1, results)
		}else{
			*results = append(*results,filepath.Join(dir,file.Name()))
		}
	}
}

func main(){
	// Nhap duong dan thu muc input
	inputhPath := ""
	fmt.Print("Nhap duong dan thu muc duyet file: ")
	fmt.Scan(&inputhPath)

	// Nhap duong dan thu muc output
	outputPath := ""
	fmt.Print("Nhap duong dan thu muc luu ket qua: ")
	fmt.Scan(&outputPath)

	// Check chuong trinh da chay truoc do chua
	_,err := os.Stat(filepath.Join(outputPath,"output.txt"))
	if err == nil {
		fmt.Println("Loi: Chuong trinh da duoc chay truoc do!")
		return
	} 

	// Tao slice luu ket qua va thuc hien duyet file
	var results []string
	listFiles(inputhPath, 1, &results)

	// Luu slcie vao output.txt 
	outputFile,err := os.Create(filepath.Join(outputPath,"output.txt"))
	if err != nil {
		fmt.Println("Loi khi tao file", err)
		return
	}

	defer outputFile.Close()

	writer := bufio.NewWriter(outputFile) // Luu du lieu vao buffer roi moi chuyen sang output.txt
	for _,result := range results {
		fmt.Fprintln(writer, result)
	}
	writer.Flush() // Dam bao du lieu tu buffer duoc ghi het xuong output file
	fmt.Println("Duyet file thanh cong, ket qua duoc luu vao: ",filepath.Join(outputPath,"output.txt"))
}