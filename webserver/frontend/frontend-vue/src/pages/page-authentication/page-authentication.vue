<template>
  <ALayout>
    <ALayoutContent :style="{ display: 'flex', flexDirection: 'column' }">
      <ARow
        type="flex"
        justify="center"
        align="middle"
        :style="{ flexGrow: 1 }"
      >
        <ACol :style="{ width: '360px' }">
          <ACard
            title="Вход"
            :head-style="{ display: 'flex', justifyContent: 'center' }"
            bordered
          >
            <AForm
              :model="formState"
              :label-col="{ span: 5 }"
              :wrapper-col="{ span: 19 }"
            >
              <AFormItem label="Логин">
                <AInput v-model:value="formState.login" />
              </AFormItem>

              <AFormItem label="Пароль">
                <AInput v-model:value="formState.password" />
              </AFormItem>

              <AFormItem
                v-if="isWrongCredentialsErrorDisplayed || isAuthenticationSucceededAlertDisplayed || isUnknownErrorDisplayed"
                :wrapper-col="{ span: 24 }"
              >
                <AAlert
                  v-if="isWrongCredentialsErrorDisplayed"
                  message="Логин или пароль неверны"
                  type="error"
                  show-icon
                />

                <AAlert
                  v-if="isUnknownErrorDisplayed"
                  message="Логин или пароль неверны"
                  type="error"
                  show-icon
                />

                <AAlert
                  v-if="isAuthenticationSucceededAlertDisplayed"
                  message="Аутенфикация прошла успешно"
                  type="success"
                  show-icon
                />
              </AFormItem>

              <AFormItem :style="{ marginBottom: 0 }" :wrapper-col="{ span: 24 }">
                <ARow justify="center">
                  <AButton
                    type="primary"
                    html-type="submit"
                    :loading="isLoading"
                    @click="login"
                  >
                    Войти
                  </AButton>
                </ARow>
              </AFormItem>
            </AForm>
          </ACard>
        </ACol>
      </ARow>
    </ALayoutContent>
  </ALayout>
</template>

<script lang="ts">
import { defineComponent, reactive, ref } from 'vue';
import { useRouter } from 'vue-router';
import BackendApiService from '../../services/backend-api-service';
import config from '../../config';
import { AuthenticationWrongCredentialsError } from '../../errors/authentication-wrong-credentials-error';

const backendApiService = new BackendApiService(config.backendApiUrl);

export default defineComponent({
  setup () {
    const formState = reactive({
      login: '',
      password: ''
    });
    const isLoading = ref(false);
    const isWrongCredentialsErrorDisplayed = ref(false);
    const isUnknownErrorDisplayed = ref(false);
    const isAuthenticationSucceededAlertDisplayed = ref(false);

    return {
      formState,
      isLoading,
      isWrongCredentialsErrorDisplayed,
      isUnknownErrorDisplayed,
      isAuthenticationSucceededAlertDisplayed
    };
  },

  methods: {
    async login () {
      this.clearErrors();

      this.isLoading = true;
      try {
        await backendApiService.authenticate(this.formState.login, this.formState.password);
        this.isAuthenticationSucceededAlertDisplayed = true;

        setTimeout(() => {
          this.$router.replace('/');
        }, 500);
      } catch (e: Error) {
        if (e instanceof AuthenticationWrongCredentialsError) {
          this.isWrongCredentialsErrorDisplayed = true;
        } else {
          this.isUnknownErrorDisplayed = true;
        }

        this.isLoading = false;
      }
    },

    clearErrors () {
      this.isUnknownErrorDisplayed = false;
      this.isWrongCredentialsErrorDisplayed = false;
    }
  }
});
</script>
