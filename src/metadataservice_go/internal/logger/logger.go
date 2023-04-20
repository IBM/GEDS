package logger

import (
	"io"
	"log"
	"os"
)

var (
	InfoLogger  *log.Logger
	ErrorLogger *log.Logger
	FatalLogger *log.Logger
)

const LogsPath = "./logs/"

func init() {
	if _, err := os.Stat(LogsPath); os.IsNotExist(err) {
		err = os.Mkdir(LogsPath, os.ModePerm)
		if err != nil {
			log.Fatal(err)
		}
	}
	logFile, err := os.OpenFile(LogsPath+"log.log", os.O_CREATE|os.O_APPEND|os.O_WRONLY, os.ModePerm)
	if err != nil {
		log.Fatal(err)
	}
	InfoLogger = log.New(io.MultiWriter(os.Stdout), "INFO: ", log.Ldate|log.Ltime|log.Lshortfile)
	multiWriter := io.MultiWriter(os.Stdout, logFile)
	ErrorLogger = log.New(multiWriter, "ERROR: ", log.Ldate|log.Ltime|log.Lshortfile)
	FatalLogger = log.New(multiWriter, "FATAL: ", log.Ldate|log.Ltime|log.Lshortfile)
}
