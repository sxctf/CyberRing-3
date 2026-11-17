package server

import (
	"github.com/gofiber/fiber/v2"
)

func emptySuccessResponse() *fiber.Map {
	return &fiber.Map{"success": true}
}

func errorResponse(err error) *fiber.Map {
	return &fiber.Map{"error": err.Error()}
}

func localUsername(c *fiber.Ctx) string {

	if v, ok := c.Locals("username").(string); ok {
		return v
	}
	return ""
}

func localIsAdmin(c *fiber.Ctx) bool {
	if v, ok := c.Locals("is_admin").(bool); ok {
		return v
	}
	return false
}

type authRequest struct {
	Username string `form:"username"`
	Password string `form:"password"`
}

func authSuccessResponse(token string) *fiber.Map {
	return &fiber.Map{
		"token": token,
	}
}

type getIdeaRequest struct {
	Title    string `form:"title"`
	Username string `form:"username"`
}

type createIdeaRequest struct {
	Status   string `form:"status"`
	Title    string `form:"title"`
	Category string `form:"category"`
	Text     string `form:"text"`
}

func ideaSuccessResponse(v map[string]string) *fiber.Map {
	return &fiber.Map{
		"status":   v["status"],
		"title":    v["title"],
		"category": v["category"],
		"text":     v["text"],
	}
}

func ideasListSuccessResponse(v []map[string]string) *fiber.Map {
	data := make([]map[string]string, 0, len(v))

	for _, j := range v {
		data = append(data, map[string]string{
			"status":   j["status"],
			"title":    j["title"],
			"category": j["category"],
			"text":     j["text"],
		})
	}

	return &fiber.Map{"data": data}
}

func ratingListSuccessResponse(v []map[string]string) *fiber.Map {
	data := make([]map[string]string, 0, len(v))

	for _, e := range v {
		data = append(data, map[string]string{
			"username":          e["username"],
			"ideas_count":       e["ideas_count"],
			"last_published_at": e["last_published_at"],
			"last_title":        e["last_title"],
			"last_category":     e["last_category"],
		})
	}

	return &fiber.Map{"data": data}
}

type createFriendRequestRequest struct {
	To string `form:"to"`
}

type acceptFriendRequestRequest struct {
	From string `form:"from"`
}

func friendsSuccessResponse(f []string) *fiber.Map {
	return &fiber.Map{"data": f}
}
