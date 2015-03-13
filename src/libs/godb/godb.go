// godb
//
// Copyright (c) 2015, Los Alamos National Security, LLC
// All rights reserved.
//
// Copyright (2015). Los Alamos National Security, LLC. This software was
// produced under U.S. Government contract DE-AC52-06NA25396 for Los Alamos
// National Laboratory (LANL), which is operated by Los Alamos National
// Security, LLC for the U.S. Department of Energy. The U.S. Government has
// rights to use, reproduce, and distribute this software. NEITHER THE
// GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY, EXPRESS
// OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE. If
// software is modified to produce derivative works, such modified software
// should be clearly marked, so as not to confuse it with the version available
// from LANL.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//
// Read and parse db data headers.
//
// Author: Curt Hash <chash@lanl.gov>

package godb

import (
	"fmt"
	"io"
	"os"
	"strings"
)

type Column struct {
	Index    int
	DataType string
}

type Schema map[string]*Column

// Returns a new Schema object.
func NewSchema() Schema {
	return make(Schema)
}

// Adds a column to the schema.
func (schema Schema) AddColumn(name string, index int, dataType string) {
	schema[name] = &Column{index, dataType}
}

// Retrieves a Column by name.
func (schema Schema) GetColumn(name string) *Column {
	if column, ok := schema[name]; ok {
		return column
	}

	return nil
}

// Boolean column name exists.
func (schema Schema) HasColumn(name string) bool {
	_, ok := schema[name]
	return ok
}

func ReadHeader(handle io.Reader) string {
	header := ""
	b := make([]byte, 1)
	for {
		handle.Read(b)
		c := string(b)
		if (c == "\n") {
			break
		}
		header += c
	}

	return header
}

// If arg is a string, it is treated as the header. If arg is an io.Reader, the
// header is first read from the Reader. Parses the db header and returns a
// Schema object.
func ParseHeader(arg interface{}) Schema {
	var header string

	switch arg.(type) {
	case io.Reader:
		header = ReadHeader(arg.(io.Reader))
	case string:
		header = arg.(string)
	default:
		fmt.Fprintln(os.Stderr, "invalid arg type")
		os.Exit(1)
	}

	schema := NewSchema()

	columns := strings.Split(header, "\t")

	if columns[0] != "#db" {
		fmt.Fprintln(os.Stderr, "missing #db magic in header")
		os.Exit(1)
	}

	for i, column := range columns[1:] {
		tokens := strings.Split(column, ":")
		if len(tokens) != 2 {
			fmt.Fprintln(os.Stderr, "invalid column definition in header", column)
			os.Exit(1)
		}

		schema.AddColumn(tokens[0], i, tokens[1])
	}

	return schema
}
