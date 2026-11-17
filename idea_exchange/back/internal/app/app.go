package app

import (
	"context"
)

type AuthUseCase interface {
	Register(ctx context.Context, username, password string) (string, error)
	Login(ctx context.Context, username, password string) (string, error)
}

type IdeasUseCase interface {
	GetIdea(ctx context.Context, requester, username, title string) (map[string]string, error)
	CreateIdea(ctx context.Context, username, status, title, category, text string) error
	GetUserIdeasList(ctx context.Context, username string) ([]map[string]string, error)
	GetCategoryIdeas(ctx context.Context, username, category string) ([]map[string]string, error)
	GetRating(ctx context.Context, sortByRecent bool, page int64) ([]map[string]string, error)
	SearchIdeas(ctx context.Context, q string) ([]map[string]string, error)
	GetIdeaByIndex(ctx context.Context, username string, idx int) (map[string]string, error)
	AdminStats(ctx context.Context) (map[string]int, error)
}

type FriendsUseCase interface {
	CreateFriendRequest(ctx context.Context, from, to string) error
	AcceptFriendRequest(ctx context.Context, username, from string) error
	GetRequestsList(ctx context.Context, username string) ([]string, error)
	GetFriendsList(ctx context.Context, username string) ([]string, error)
}

type App struct {
	Auth    AuthUseCase
	Ideas   IdeasUseCase
	Friends FriendsUseCase
}

func New(auth AuthUseCase, ideas IdeasUseCase, friends FriendsUseCase) App {
	return App{
		Auth:    auth,
		Ideas:   ideas,
		Friends: friends,
	}
}
