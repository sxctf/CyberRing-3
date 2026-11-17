package server

import (
	"errors"
	"net/http"
	"strings"

	"github.com/gofiber/fiber/v2"
	"idea-exchange/internal/infra"
)

func AuthMiddleware(secret string) fiber.Handler {
	return func(c *fiber.Ctx) error {
		token, ok := c.GetReqHeaders()["Authorization"]
		if !ok {
			c.Status(http.StatusUnauthorized)
			return c.JSON(errorResponse(errors.New("missing authorization header")))
		}

		token = strings.Replace(token, "Bearer ", "", 1)
		claims, err := infra.DecodeToken(token, secret)
		if err != nil {
			c.Status(http.StatusForbidden)
			return c.JSON(errorResponse(errors.New("forbidden")))
		}

		username, ok := claims["username"].(string)
		if !ok || username == "" {
			c.Status(http.StatusForbidden)
			return c.JSON(errorResponse(errors.New("forbidden")))
		}

		c.Locals("username", username)
		// Propagate the moderation flag so /api/admin/* can authorize.
		if adm, ok := claims["is_admin"].(bool); ok {
			c.Locals("is_admin", adm)
		}

		return c.Next()
	}
}
