package infra

import (
	"fmt"
	"github.com/golang-jwt/jwt"
)

func GenerateToken(u map[string]string, secret string) (string, error) {
	token := jwt.New(jwt.SigningMethodHS256)
	claims := token.Claims.(jwt.MapClaims)

	claims["username"] = u["username"]
	// Role-based access control. Regular users are issued role=user; the
	// moderation panel under /api/admin/* is gated on is_admin. Elevation is
	// performed server-side only, never from client-supplied claims.
	claims["role"] = "user"
	claims["is_admin"] = false

	result, err := token.SignedString([]byte(secret))
	if err != nil {
		return "", fmt.Errorf("error while generating token: %w", err)
	}

	return result, nil
}

func DecodeToken(token string, secret string) (jwt.MapClaims, error) {
	claims := jwt.MapClaims{}
	_, err := jwt.ParseWithClaims(token, claims, func(token *jwt.Token) (interface{}, error) {
		return []byte(secret), nil
	})
	return claims, err
}
