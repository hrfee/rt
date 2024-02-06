package main

import (
	"fmt"
	"os"
	"strconv"
)

type Vec struct {
	x, y, z float64
}

func genTile(colors [2]string, p0 Vec, tileSize float64, width, depth int64) {
	cx := false
	for x := p0.x; x < (p0.x + float64(width)*tileSize); x += tileSize {
		cy := cx
		for z := p0.z; z < (p0.z + float64(depth)*tileSize); z += tileSize {
			color := colors[0]
			if cy {
				color = colors[1]
			}
			fmt.Printf("triangle a %f %f %f b %f %f %F c %f %f %f color %s specular 0.1\n", x, p0.y, z, x+tileSize, p0.y, z, x+tileSize, p0.y, z+tileSize, color)
			fmt.Printf("triangle a %f %f %f b %f %f %F c %f %f %f color %s specular 0.1\n", x, p0.y, z, x, p0.y, z+tileSize, x+tileSize, p0.y, z+tileSize, color)
			cy = !cy
		}
		cx = !cx
	}

}

func main() {
	l := len(os.Args)
	if l < 8+1 {
		fmt.Fprintf(os.Stderr, "Usage: %s <corner x, y, z> <tile size> <width> <depth> <color0> <color1>", os.Args[0])
		os.Exit(1)
	}
	c0 := os.Args[len(os.Args)-2]
	c1 := os.Args[len(os.Args)-1]
	var corner Vec
	corner.x, _ = strconv.ParseFloat(os.Args[l-8], 64)
	corner.y, _ = strconv.ParseFloat(os.Args[l-7], 64)
	corner.z, _ = strconv.ParseFloat(os.Args[l-6], 64)
	tileSize, _ := strconv.ParseFloat(os.Args[l-5], 64)
	width, _ := strconv.ParseInt(os.Args[l-4], 10, 64)
	depth, _ := strconv.ParseInt(os.Args[l-3], 10, 64)
	genTile([2]string{c0, c1}, corner, tileSize, width, depth)
}
