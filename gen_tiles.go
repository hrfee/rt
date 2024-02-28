package main

import (
	"fmt"
	"os"
	"strconv"
)

type Vec struct {
	x, y, z float64
}

func genAAB(color string, p0, p1 Vec) {
	points := []Vec{p0, p0, p1, p1}
	if p0.z == p1.z || p0.y == p1.y {
		points[1].x = p1.x
		points[3].x = p0.x
	} else if p0.x == p1.x {
		points[1].z = p1.z
		points[3].z = p0.z
	}

	fmt.Printf("triangle a %f %f %f b %f %f %f c %f %f %f color %s reflectiveness 0.1\n", points[0].x, points[0].y, points[0].z, points[1].x, points[1].y, points[1].z, points[2].x, points[2].y, points[2].z, color)
	fmt.Printf("triangle a %f %f %f b %f %f %f c %f %f %f color %s reflectiveness 0.1\n", points[2].x, points[2].y, points[2].z, points[3].x, points[3].y, points[3].z, points[0].x, points[0].y, points[0].z, color)
}

func genTile(colors [2]string, p0 Vec, tileSize float64, width, depth int64, ceilingHeight float64, ceilingColor string) {
	maxX := p0.x + float64(width)*tileSize
	maxZ := p0.z + float64(depth)*tileSize
	fmt.Printf("container a %f %f %f b %f %f %f c %f %f %f d %f %f %f {\n", p0.x, p0.y, p0.z, maxX, p0.y, p0.z, maxX, p0.y, maxZ, p0.x, p0.y, maxZ)
	cx := false
	for x := p0.x; x < maxX; x += tileSize {
		cy := cx
		for z := p0.z; z < maxZ; z += tileSize {
			color := colors[0]
			if cy {
				color = colors[1]
			}
			fmt.Printf("\ttriangle a %f %f %f b %f %f %f c %f %f %f color %s reflectiveness 0.1\n", x, p0.y, z, x+tileSize, p0.y, z, x+tileSize, p0.y, z+tileSize, color)
			fmt.Printf("\ttriangle a %f %f %f b %f %f %f c %f %f %f color %s reflectiveness 0.1\n", x, p0.y, z, x, p0.y, z+tileSize, x+tileSize, p0.y, z+tileSize, color)
			cy = !cy
		}
		cx = !cx
	}
	fmt.Printf("}\n")
	if ceilingHeight == 0 {
		return
	}
	fmt.Printf("\n\n// Wall\n\n")
	pa := p0
	pb := Vec{maxX, ceilingHeight, p0.z}
	genAAB(ceilingColor, pa, pb)
	pb = Vec{p0.x, ceilingHeight, maxZ}
	genAAB(ceilingColor, pa, pb)
	pa = Vec{maxX, p0.y, p0.z}
	pb = Vec{maxX, ceilingHeight, maxZ}
	genAAB(ceilingColor, pa, pb)
	pa = Vec{maxX, p0.y, maxZ}
	pb = Vec{p0.x, ceilingHeight, maxZ}
	genAAB(ceilingColor, pa, pb)
	pa = Vec{p0.x, ceilingHeight, p0.z}
	pb = Vec{maxX, ceilingHeight, maxZ}
	genAAB(ceilingColor, pa, pb)
}

func main() {
	l := len(os.Args)
	if l < 8+1 {
		fmt.Fprintf(os.Stderr, "Usage: %s <corner x, y, z> <tile size> <width> <depth> <color0> <color1> <ceilingHeight> <ceilingColor>", os.Args[0])
		os.Exit(1)
	}
	ceilingHeight, _ := strconv.ParseFloat(os.Args[l-2], 64)
	ceilingColor := os.Args[l-1]
	c0 := os.Args[len(os.Args)-4]
	c1 := os.Args[len(os.Args)-3]
	var corner Vec
	corner.x, _ = strconv.ParseFloat(os.Args[l-10], 64)
	corner.y, _ = strconv.ParseFloat(os.Args[l-9], 64)
	corner.z, _ = strconv.ParseFloat(os.Args[l-8], 64)
	tileSize, _ := strconv.ParseFloat(os.Args[l-7], 64)
	width, _ := strconv.ParseInt(os.Args[l-6], 10, 64)
	depth, _ := strconv.ParseInt(os.Args[l-5], 10, 64)
	genTile([2]string{c0, c1}, corner, tileSize, width, depth, ceilingHeight, ceilingColor)
}
