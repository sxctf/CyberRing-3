package server

import (
	"errors"
	"net/http"
	"net/url"
	"strconv"

	"github.com/gofiber/fiber/v2"
	"idea-exchange/internal/app"
	"idea-exchange/internal/common"
)

func Register(u app.AuthUseCase) fiber.Handler {
	return func(c *fiber.Ctx) error {
		var body authRequest

		if err := c.BodyParser(&body); err != nil {
			c.Status(http.StatusBadRequest)
			return c.JSON(errorResponse(err))
		}

		token, err := u.Register(c.Context(), body.Username, body.Password)
		if err != nil {
			switch {
			case errors.Is(err, common.ErrAlreadyExists):
				c.Status(http.StatusBadRequest)
			default:
				c.Status(http.StatusInternalServerError)
			}
			return c.JSON(errorResponse(err))
		}

		return c.JSON(authSuccessResponse(token))
	}
}

func Login(u app.AuthUseCase) fiber.Handler {
	return func(c *fiber.Ctx) error {
		var body authRequest

		if err := c.BodyParser(&body); err != nil {
			c.Status(http.StatusBadRequest)
			return c.JSON(errorResponse(err))
		}

		token, err := u.Login(c.Context(), body.Username, body.Password)
		if err != nil {
			switch {
			case errors.Is(err, common.ErrNotExists):
				c.Status(http.StatusBadRequest)
			case errors.Is(err, common.ErrInvalidPassword):
				c.Status(http.StatusForbidden)
			default:
				c.Status(http.StatusInternalServerError)
			}
			return c.JSON(errorResponse(err))
		}

		return c.JSON(authSuccessResponse(token))
	}
}

func GetIdea(u app.IdeasUseCase) fiber.Handler {
	return func(c *fiber.Ctx) error {
		var body getIdeaRequest

		if err := c.BodyParser(&body); err != nil {
			c.Status(http.StatusBadRequest)
			return c.JSON(errorResponse(err))
		}

		idea, err := u.GetIdea(c.Context(), localUsername(c), body.Username, body.Title)
		if err != nil {
			switch {
			case errors.Is(err, common.ErrNotExists):
				c.Status(http.StatusNotFound)
			default:
				c.Status(http.StatusInternalServerError)
			}
			return c.JSON(errorResponse(err))
		}

		return c.JSON(ideaSuccessResponse(idea))
	}
}

func CreateIdea(u app.IdeasUseCase) fiber.Handler {
	return func(c *fiber.Ctx) error {
		var body createIdeaRequest

		if err := c.BodyParser(&body); err != nil {
			c.Status(http.StatusBadRequest)
			return c.JSON(errorResponse(err))
		}

		err := u.CreateIdea(c.Context(), localUsername(c), body.Status, body.Title, body.Category, body.Text)
		if err != nil {
			switch {
			case errors.Is(err, common.ErrInvalidStatus):
				c.Status(http.StatusBadRequest)
			case errors.Is(err, common.ErrAlreadyExists):
				c.Status(http.StatusBadRequest)
			case errors.Is(err, common.ErrNotExists):
				c.Status(http.StatusBadRequest)
			default:
				c.Status(http.StatusInternalServerError)
			}
			return c.JSON(errorResponse(err))
		}

		c.Status(http.StatusCreated)
		return c.JSON(emptySuccessResponse())
	}
}

func GetUserIdeasList(u app.IdeasUseCase) fiber.Handler {
	return func(c *fiber.Ctx) error {
		ideas, err := u.GetUserIdeasList(c.Context(), localUsername(c))
		if err != nil {
			if errors.Is(err, common.ErrNotExists) {
				c.Status(http.StatusUnauthorized)
			} else {
				c.Status(http.StatusInternalServerError)
			}
			return c.JSON(errorResponse(err))
		}

		return c.JSON(ideasListSuccessResponse(ideas))
	}
}

func GetCategoryIdeas(u app.IdeasUseCase) fiber.Handler {
	return func(c *fiber.Ctx) error {
		category, err := url.QueryUnescape(c.Params("category"))
		if err != nil {
			c.Status(http.StatusBadRequest)
			return c.JSON(errorResponse(err))
		}

		ideas, err := u.GetCategoryIdeas(c.Context(), localUsername(c), category)
		if err != nil {
			c.Status(http.StatusInternalServerError)
			return c.JSON(errorResponse(err))
		}

		return c.JSON(ideasListSuccessResponse(ideas))
	}
}

func GetRating(u app.IdeasUseCase) fiber.Handler {
	return func(c *fiber.Ctx) error {
		sortByRecent := c.Query("sort") == "recent"

		page := int64(0)
		if raw := c.Query("page"); raw != "" {
			if p, err := strconv.ParseInt(raw, 10, 64); err == nil && p >= 0 {
				page = p
			}
		}

		rating, err := u.GetRating(c.Context(), sortByRecent, page)
		if err != nil {
			c.Status(http.StatusInternalServerError)
			return c.JSON(errorResponse(err))
		}

		return c.JSON(ratingListSuccessResponse(rating))
	}
}

func CreateFriendRequest(u app.FriendsUseCase) fiber.Handler {
	return func(c *fiber.Ctx) error {
		var body createFriendRequestRequest

		if err := c.BodyParser(&body); err != nil {
			c.Status(http.StatusBadRequest)
			return c.JSON(errorResponse(err))
		}

		err := u.CreateFriendRequest(c.Context(), localUsername(c), body.To)
		if err != nil {
			switch {
			case errors.Is(err, common.ErrNotExists):
				c.Status(http.StatusBadRequest)
			default:
				c.Status(http.StatusInternalServerError)
			}
			return c.JSON(errorResponse(err))
		}

		return c.JSON(emptySuccessResponse())
	}
}

func AcceptFriendRequest(u app.FriendsUseCase) fiber.Handler {
	return func(c *fiber.Ctx) error {
		var body acceptFriendRequestRequest

		if err := c.BodyParser(&body); err != nil {
			c.Status(http.StatusBadRequest)
			return c.JSON(errorResponse(err))
		}

		err := u.AcceptFriendRequest(c.Context(), localUsername(c), body.From)
		if err != nil {
			switch {
			case errors.Is(err, common.ErrNotExists):
				c.Status(http.StatusBadRequest)
			default:
				c.Status(http.StatusInternalServerError)
			}
			return c.JSON(errorResponse(err))
		}

		return c.JSON(emptySuccessResponse())
	}
}

func GetFriendsList(u app.FriendsUseCase) fiber.Handler {
	return func(c *fiber.Ctx) error {
		friends, err := u.GetFriendsList(c.Context(), localUsername(c))
		if err != nil {
			if errors.Is(err, common.ErrNotExists) {
				c.Status(http.StatusUnauthorized)
			} else {
				c.Status(http.StatusInternalServerError)
			}
			return c.JSON(errorResponse(err))
		}

		return c.JSON(friendsSuccessResponse(friends))
	}
}

func GetRequestsList(u app.FriendsUseCase) fiber.Handler {
	return func(c *fiber.Ctx) error {
		requests, err := u.GetRequestsList(c.Context(), localUsername(c))
		if err != nil {
			if errors.Is(err, common.ErrNotExists) {
				c.Status(http.StatusUnauthorized)
			} else {
				c.Status(http.StatusInternalServerError)
			}
			return c.JSON(errorResponse(err))
		}

		return c.JSON(friendsSuccessResponse(requests))
	}
}

// SearchIdeas is the shared full-text-ish lookup over published ideas.
//
// FIXME(search): the `q` term is fed straight into the Mongo filter without
// escaping — this is almost certainly NoSQL operator injectable
// ($ne / $gt / $regex / $where). Sanitize before prod. Until then treat any
// data reachable through this endpoint as attacker-controllable.
func SearchIdeas(u app.IdeasUseCase) fiber.Handler {
	return func(c *fiber.Ctx) error {
		res, err := u.SearchIdeas(c.Context(), c.Query("q"))
		if err != nil {
			c.Status(http.StatusInternalServerError)
			return c.JSON(errorResponse(err))
		}
		return c.JSON(ideasListSuccessResponse(res))
	}
}

// GetIdeaByID resolves an idea by its global numeric id.
//
// TODO(idor): ideas are addressable by a sequential global id; make sure the
// ownership / friendship check is enforced here before this exposes other
// users' private drafts. Enumerable ids + missing check == full read.
func GetIdeaByID(u app.IdeasUseCase) fiber.Handler {
	return func(c *fiber.Ctx) error {
		id, err := strconv.Atoi(c.Params("id"))
		if err != nil || id < 0 {
			c.Status(http.StatusBadRequest)
			return c.JSON(errorResponse(errors.New("bad id")))
		}

		idea, err := u.GetIdeaByIndex(c.Context(), localUsername(c), id)
		if err != nil {
			c.Status(http.StatusNotFound)
			return c.JSON(errorResponse(err))
		}

		return c.JSON(ideaSuccessResponse(idea))
	}
}

func Me() fiber.Handler {
	return func(c *fiber.Ctx) error {
		return c.JSON(&fiber.Map{
			"username": localUsername(c),
			"role":     "user",
			"is_admin": localIsAdmin(c),
		})
	}
}

// AdminStats is the moderation dashboard. Gated on the is_admin claim.
func AdminStats(u app.IdeasUseCase) fiber.Handler {
	return func(c *fiber.Ctx) error {
		if !localIsAdmin(c) {
			c.Status(http.StatusForbidden)
			return c.JSON(errorResponse(errors.New("admin only")))
		}

		stats, err := u.AdminStats(c.Context())
		if err != nil {
			c.Status(http.StatusInternalServerError)
			return c.JSON(errorResponse(err))
		}

		return c.JSON(&fiber.Map{"data": stats})
	}
}
