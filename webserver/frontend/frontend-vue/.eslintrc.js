module.exports = {
  root: true,
  extends: [
    '@nuxtjs/eslint-config-typescript'
  ],
  env: {
    node: true,
    browser: true
  },
  globals: {
    RequestInit: 'readonly'
  },
  settings: {
    'import/resolver': {
      typescript: {
        alwaysTryTypes: true
      }
    }
  },
  rules: {
    semi: ['error', 'always'],
    'require-await': 'off',
    // @todo: remove nuxtjs and write own config
    'vue/no-v-model-argument': 'off'
  },
  overrides: [
    {
      files: ['*.ts'],
      rules: {
        'no-undef': 'off'
      }
    }
  ]
};
