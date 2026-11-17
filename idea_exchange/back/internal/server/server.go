package server

import (
	"github.com/gofiber/fiber/v2"
	"github.com/gofiber/fiber/v2/middleware/recover"
	"idea-exchange/internal/app"
)

func NewServer(port, secret string, a app.App) *Server {

	app := fiber.New(fiber.Config{
		DisableStartupMessage: true,
	})
	app.Use(recover.New())

	s := &Server{port: port, app: app}
	api := s.app.Group("/api")

	api.Post("/register", Register(a.Auth))
	api.Post("/login", Login(a.Auth))

	auth := api.Group("/")
	auth.Use(AuthMiddleware(secret))

	ideas := auth.Group("/ideas")
	{
		ideas.Get("/", GetIdea(a.Ideas))
		ideas.Post("/", CreateIdea(a.Ideas))
		ideas.Get("/list", GetUserIdeasList(a.Ideas))
		ideas.Get("/rating", GetRating(a.Ideas))
		ideas.Get("/search", SearchIdeas(a.Ideas))
		ideas.Get("/by-id/:id", GetIdeaByID(a.Ideas))
		ideas.Get("/list/:category", GetCategoryIdeas(a.Ideas))
	}

	auth.Get("/me", Me())

	admin := auth.Group("/admin")
	{
		admin.Get("/stats", AdminStats(a.Ideas))
	}

	friends := auth.Group("/friends")
	{
		friends.Get("/", GetFriendsList(a.Friends))
		friends.Get("/requests", GetRequestsList(a.Friends))
		friends.Post("/request", CreateFriendRequest(a.Friends))
		friends.Post("/accept", AcceptFriendRequest(a.Friends))
	}

	return s
}

type Server struct {
	port string
	app  *fiber.App
}

func (s *Server) Run() error {
	return s.app.Listen(s.port)
}
