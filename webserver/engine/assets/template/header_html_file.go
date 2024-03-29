package template

var VarHeaderHtmlFile = []byte(`<!doctype html>
<html lang="en">
	<head>
		<!-- Required meta tags -->
		<meta charset="utf-8" />
		<meta name="theme-color" content="#205081" />
		<meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no" />

		<!-- Bootstrap CSS -->
		<link rel="stylesheet" href="{{$.System.PathCssBootstrap}}" />
		<link rel="stylesheet" href="{{$.System.PathCssMaterilaDashboard}}">
		<link rel="stylesheet" href="{{$.System.PathCssLightGallery}}" />

		<title>{{$.Data.CachedBlock1}}</title>
		<meta name="keywords" content="{{$.Data.Page.MetaKeywords}}" />
		<meta name="description" content="{{$.Data.Page.MetaDescription}}" />
		<link rel="shortcut icon" href="{{$.System.PathIcoFav}}" type="image/x-icon" />

		<!-- Template CSS file from template folder -->
		<!--<link rel="stylesheet" href="{{$.System.PathThemeStyles}}" />-->
		<link rel="stylesheet" type="text/css" href="{{$.System.PathCssGoogleFonts}}" />
		<link rel="stylesheet" href="{{$.System.PathCssAvesomeFonts}}" />
	</head>
	<body id="body" class="fixed-top-bar">
		<div id="sys-modal-shop-basket-placeholder"></div>
		<div id="wrap">
			<nav id="navbar-top" class="navbar navbar-expand-lg navbar-light bg-light">
				<div class="container">
					<a class="navbar-brand" href="/">КОТМИ WEB {{$.System.InfoVersion}}</a>
					<button class="navbar-toggler collapsed" type="button" data-toggle="collapse" data-target="#navbarResponsive" aria-controls="navbarResponsive" aria-expanded="false" aria-label="Toggle navigation">
						<span class="navbar-toggler-icon"></span>
					</button>
					<div class="collapse navbar-collapse" id="navbarResponsive">
						<ul class="navbar-nav ml-auto">
							<li class="nav-item{{if eq $.Data.Page.Alias "/"}} active{{end}}">
								<a class="nav-link" href="/">Home</a>
							</li>
							<li class="nav-item">
								<a class="nav-link{{if eq $.Data.Page.Alias "/another/"}} active{{end}}" href="/another/">Another</a>
							</li>
							<li class="nav-item">
								<a class="nav-link{{if eq $.Data.Page.Alias "/about/"}} active{{end}}" href="/about/">About</a>
							</li>
							<li class="nav-item">
								<a class="nav-link{{if eq $.Data.Page.Alias "/dashboard/"}} active{{end}}" href="/dashboard/">Дашборд</a>
							</li>
							<li class="nav-item">
								<a class="nav-link{{if eq $.Data.Module "404"}} active{{end}}" href="/not-existent-page/">404</a>
							</li>
						</ul>
						<ul class="navbar-nav ml-auto">
							<li class="nav-item dropdown"> 
								<a class="nav-link dropdown-toggle" href="javascript:;" id="nbAccountDropdown" role="button" data-toggle="dropdown" aria-haspopup="true" aria-expanded="false">
									{{$.Data.UserEmail}}
								</a>
								<div class="dropdown-menu dropdown-menu-right" aria-labelledby="nbAccountDropdown"><a class="dropdown-item" href="javascript:fave.ModalUserProfile();">Профиль</a>
								<div class="dropdown-divider"></div><a class="dropdown-item" href="javascript:fave.ActionLogout('Are you sure want to logout?', '/');">Выход</a></div>
							</li>
						</ul>
					</div>
				</div>
			</nav>
			<div id="main">
				<div class="bg-fave">
					<div class="container">
						<h1 class="text-left text-white m-0 p-0 py-5">{{$.Data.CachedBlock2}}</h1>
					</div>
				</div>
				{{$.Data.CachedBlock3}}
				<div class="container clear-top">
					<div class="row pt-4">
						{{if or (eq $.Data.Module "shop") (eq $.Data.Module "shop-category")}}
							<div class="col-sm-5 col-md-4 col-lg-3">
								{{template "sidebar-left.html" .}}
							</div>
						{{end}}
						{{if or (eq $.Data.Module "shop-product")}}
							<div class="col-md-12">
						{{else}}
							<div class="col-sm-7 col-md-8 col-lg-9">
						{{end}}`)
