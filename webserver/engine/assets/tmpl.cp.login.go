package assets

var TmplCpLogin = []byte(`<!doctype html>
<html lang="en">
	<head>
		<meta charset="utf-8">
		<meta name="theme-color" content="#205081" />
		<meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no">
		<title>Please login</title>
		<link rel="stylesheet" href="{{$.System.PathCssBootstrap}}">
		<link rel="stylesheet" href="{{$.System.PathCssCpStyles}}">
		<link rel="shortcut icon" href="{{$.System.PathIcoFav}}" type="image/x-icon" />
		<style type="text/css">html{height:100%;}</style>
	</head>
	<body class="cp-login text-center">
		<form class="form-signin card" action="/cp/" method="post">
			<input type="hidden" name="action" value="index-user-sign-in">
			<h1 class="h3 mb-3 font-weight-normal">Авторизация</h1>
			<label for="email" class="sr-only">Пользователь</label>
			<input type="text" id="first_name" name="first_name" class="form-control" placeholder="Пользователь" required autofocus>
			<label for="password" class="sr-only">Пароль</label>
			<input type="password" id="password" name="password" class="form-control mb-3" placeholder="Пароль" required>
			<div class="sys-messages"></div>
			<button class="btn btn-lg btn-primary btn-block" type="submit" formnovalidate>Вход</button>
			<div class="sys-back-link"><a href="/">&larr; Вернутся на главную страницу</a></div>
		</form>
		<script src="{{$.System.PathJsJquery}}"></script>
		<script src="{{$.System.PathJsPopper}}"></script>
		<script src="{{$.System.PathJsBootstrap}}"></script>
		<script src="{{$.System.PathJsCpScripts}}"></script>
	</body>
</html>`)
