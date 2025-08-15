package main

import (
	"log"
	"os"
	"strings"
)

func main() {
	log.SetFlags(0)
	log.SetOutput(os.Stdout)
	var readmeVersion string
	{
		text := readFile("README.md")
		_, text = mustCut(text, "\nChangelog")
		_, text = mustCut(text, "### v")
		readmeVersion, _ = mustCut(text, "\n")
	}
	var sourceVersion string
	{
		// #define SQINN_VERSION "2.0.0"
		text := readFile("lib/main.c")
		_, text = mustCut(text, "#define SQINN_VERSION \"")
		sourceVersion, _ = mustCut(text, "\"")
	}
	if readmeVersion != sourceVersion {
		log.Fatal("not OK: version mismatch")
		log.Printf("readmeVersion %q", readmeVersion)
		log.Printf("sourceVersion %q", sourceVersion)
	}
	log.Print("check.go ok")
}

func readFile(name string) string {
	return string(must(os.ReadFile(name)))
}

func mustCut(s, sep string) (_before string, _after string) {
	before, after, ok := strings.Cut(s, sep)
	if !ok {
		log.Fatalf("%q not found in %q", sep, s)
	}
	return strings.TrimSpace(before), strings.TrimSpace(after)
}

func must[V any](v V, err error) V {
	if err != nil {
		log.Fatal(err)
	}
	return v
}
