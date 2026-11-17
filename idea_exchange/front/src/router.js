import {createRouter, createWebHashHistory} from "vue-router";
import HomeView from "@/views/HomeView.vue";
import RegisterView from "@/views/RegisterView.vue";
import LoginView from "@/views/LoginView.vue";
import IdeaFormView from "@/views/IdeaFormView.vue";
import MyIdeasView from "@/views/MyIdeasView.vue";
import IdeaSearchView from "@/views/IdeaSearchView.vue";
import RatingView from "@/views/RatingView.vue";
import ProfileView from "@/views/ProfileView.vue";

const routes = [
    { path: '/', component: HomeView },
    { path: '/register', component: RegisterView },
    { path: '/profile', component: ProfileView },
    { path: '/login', component: LoginView },
    { path: '/idea/new', component: IdeaFormView },
    { path: '/idea/my', component: MyIdeasView },
    { path: '/ideas', component: IdeaSearchView },
    { path: '/rating', component: RatingView },
]

const router = createRouter({
    history: createWebHashHistory(),
    routes,
})

export default router;
