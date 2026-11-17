<template>
  <div class="container">
      <div class="searching">
          <va-input
                  v-model="search"
                  class="mb-6 search"
                  label="Search by category"
                  @keyup.enter="searchIdeas"
          >
              <template #appendInner>
                  <va-icon         name="search" />
              </template>
          </va-input>
          <va-button @click="searchIdeas">
              Find
          </va-button>
      </div>

      <div class="ideas">

          <va-progress-circle v-if="loading" indeterminate color="primary" />
          <p v-if="!loading && ideas.length === 0">Nothing found. Try changing your search.</p>
          <IdeaCard v-for="(idea, i) in ideas" :idea="idea" :key="i"/>
      </div>
  </div>
</template>

<script>
import IdeaCard from "@/components/IdeaCard.vue";
import {searchIdeas} from "@/api/ideas";

export default {
    name: "IdeaSearchView",
    components: {IdeaCard},
    props:['token'],
    data() {
        return {
            loading: false,
            search: "",
            ideas: [
            ]
        }
    },
    methods: {
      searchIdeas(){
          this.loading = true;
          searchIdeas(this.token, this.search).then((ideas) =>{
            this.ideas = ideas || [];
            this.loading = false;
          }).catch((error) => this.$vaToast.init({ message: String(error), position: 'bottom-right' }))
      }
    },
}
</script>

<style scoped>
 .container {
     margin: 30px ;
 }
 .searching{
     width: calc(100% - 60px);
     margin: 0 30px ;
     display: flex;
     gap: 30px;
 }
 .search {
    width: 100%;
 }
 .ideas{
     display: flex;
     align-items: center;
     flex-direction: column;
     margin-top: 30px;
 }
</style>
