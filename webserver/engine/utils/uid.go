package utils

import (
	"fmt"
	"math/rand"
	"strconv"
	"time"
)

func GetUID(size int) int64 {

	rand.Seed(time.Now().UnixNano())

	var matrix string = ""
	for i := 0; i < size; i++ {
		min := 0
		max := 9
		if i == 0 {
			min = 1
		}
		r := rand.Intn(max-min) + min
		matrix += strconv.Itoa(r)
	}

	result, err := strconv.ParseInt(matrix, 10, 64)

	if err != nil {
		fmt.Println("Ошибка получения UID: ", err.Error())
		return -1
	}

	return result
}
