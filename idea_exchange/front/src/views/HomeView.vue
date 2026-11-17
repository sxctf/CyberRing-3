<template>
    <div class="container">
        <div class="centers">
            <h1>Welcome to the Idea Exchange</h1>
            <h1>Authors publish their ideas here and share them with friends</h1>
            <h1>Private ideas are visible only to friends, public ones to everyone</h1>
            <h1>Compete on the leaderboard: the more ideas you post and the fresher your latest, the higher you rank</h1>
            <h1>Publish, find like-minded people, and get inspired!</h1>
        </div>
    </div>
</template>

<script>
export default {
    name: "HomeView"
}
</script>

<style scoped>
.container {
    display: flex;
    justify-content: flex-end;
    align-items: center;
    flex-direction: column;
    height: 88vh;
    background-image: linear-gradient(to bottom,
        rgba(246, 246, 246, 0) 0%,
        rgba(246, 246, 246, 0) 42%,
        rgba(246, 246, 246, 0.9) 70%,
        rgba(246, 246, 246, 1) 86%),
        url("@/assets/hero.png");
    background-size: cover;
    background-position: center top;
    padding: 20px 20px 6vh;
    margin-top: -20px;
}
.centers {
    text-align: center;
    margin-bottom: 15px;
}
h1 {
    font-size: 24px;
    margin: 6px 0;
}
</style>
