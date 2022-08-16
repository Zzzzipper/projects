// Snowpack Configuration File
// See all supported options: https://www.snowpack.dev/reference/configuration

/** @type {import("snowpack").SnowpackUserConfig } */
module.exports = {
  mount: {
    /* ... */
  },
  plugins: [
    '@snowpack/plugin-vue',
    '@snowpack/plugin-typescript',
    '@snowpack/plugin-dotenv',
    // ['@snowpack/plugin-babel'],
    [
      'snowpack-plugin-less',
      {
        javascriptEnabled: true
      }]
  ],
  packageOptions: {
    /* ... */
  },
  devOptions: {
    sourcemap: 'inline',
  },
  buildOptions: {
    sourcemap: 'inline',
  },
  routes: [{ match: 'routes', src: '.*', dest: '/index.html' }]
};
