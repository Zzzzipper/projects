package utils

import (
	"github.com/google/uuid"
)

// Token структура
type Token struct {
	token         string `json:token`
	refreshTocken string `json:refreshToken`
}

// Body
type Body struct {
	body Token `json:body`
}

func Uuid() string {
	return uuid.New().String()
}
