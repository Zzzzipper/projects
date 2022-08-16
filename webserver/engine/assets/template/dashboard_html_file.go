package template

var VarDashboardHtmlFile = []byte(`
<!doctype html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta name="theme-color" content="#205081"/>
    <meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no">
    <!--<title>{{$.Data.CachedBlock1}}</title>-->
	<title>Панель управления</title>
    <link rel="stylesheet" href="{{$.System.PathCssBootstrap}}">
	<link rel="stylesheet" href="{{$.System.PathCssBootstrapMin}}">
	<link rel="stylesheet" href="{{$.System.PathCssMaterilaDashboard}}">
    {{if or (eq $.System.CpModule "dashboard") (eq $.System.CpModule "index") (eq $.System.CpModule "blog") (eq $.System.CpModule "shop")}}
        {{if or (eq $.System.CpSubModule "add") (eq $.System.CpSubModule "modify")}}
            <link rel="stylesheet" href="{{$.System.PathCssCpWysiwygPell}}">
        {{end}}
    {{end}}
    {{if eq $.System.CpModule "templates"}}
        <link rel="stylesheet" href="{{$.System.PathCssCpCodeMirror}}">
    {{end}}
    <!--<link rel="stylesheet" href="{{$.System.PathCssStyles}}" />-->
    <link rel="stylesheet" href="{{$.System.PathCssCpStyles}}"/>
    <link rel="stylesheet" type="text/css" href="{{$.System.PathCssGoogleFonts}}"/>
    <link rel="stylesheet" href="{{$.System.PathCssAvesomeFonts}}"/>
    <link rel="stylesheet" href="{{$.System.PathMaterialDemoCss}}"/>
	<link rel="stylesheet" href="{{$.System.PathSuiteCss}}"/>
	<link rel="stylesheet" href="{{$.System.PathGridCss}}"/>
	<link rel="stylesheet" href="{{$.System.PathChartMinCss}}"/>
    <link rel="shortcut icon" href="{{$.System.PathIcoFav}}" type="image/x-icon"/>
	<script src="{{$.System.PathUtilsJs}}"></script>
	<script src="{{$.System.PathSuiteJs}}"></script>
	<script src="{{$.System.PathDatasetJs}}"></script>
    <script type="text/javascript">
        var CurrentUserProfileData = {
            first_name: '{{$.Data.UserFirstName}}',
            last_name: '{{$.Data.UserLastName}}',
            email: '{{$.Data.UserEmail}}',
			address: '{{$.Data.ServerAddress}}',
			port: '{{$.Data.ServerPort}}'
        };

        function WaitForFave(callback) {
            if (window && window.fave) {
                callback();
            } else {
                setTimeout(function () {
                    WaitForFave(callback);
                }, 100);
            }
        }
    </script>
</head>
<body>
<div class="wrapper ">
    <div class="sidebar" data-color="purple" data-background-color="white" data-image="{{$.System.PathImageSidebarJpeg1}}">
        <!--
              Tip 1: You can change the color of the sidebar using: data-color="purple | azure | green | orange | danger"
              Tip 2: you can also add an image using data-image tag
        -->
        <div class="logo"><a href="#" class="simple-text logo-normal text-center"> Котми WEB </a></div>
        <div class="sidebar-wrapper">
			<!-- cp block -->
			<!--<div class="scroll">
            	<div class="padd">-->
					{{$.Data.Dashboard.LeftSideBar}}
				<!--</div>
        	</div>-->
			<!-- cp block -->
        </div>
		<div class="sidebar-background" style="background-image: url({{$.System.PathImageSidebarJpeg1}}) "></div>
    </div>
    <div class="main-panel">
        <!-- Navbar -->
        <nav class="navbar navbar-expand-lg navbar-transparent navbar-absolute fixed-top ">
            <div class="container-fluid">
                <div class="navbar-wrapper">
					<div class="navbar-minimize">
              			<button id="minimizeSidebar" class="btn btn-just-icon btn-white btn-fab btn-round">
                			<i class="material-icons notranslate text_align-center visible-on-sidebar-regular">more_vert</i>
                			<i class="material-icons notranslate design_bullet-list-67 visible-on-sidebar-mini">view_list</i>
              				<div class="ripple-container"></div>
						</button>
            		</div>
					<!--<a class="navbar-brand" href="/cp/">{{/*$.Data.Caption*/}}&nbsp;Администрирование</a>-->
				</div>
				<button class="navbar-toggler" type="button" data-toggle="collapse" aria-controls="navigation-index"
                        aria-expanded="false" aria-label="Toggle navigation">
					<span class="sr-only">Toggle navigation</span> <span class="navbar-toggler-icon icon-bar"></span>
                	<span class="navbar-toggler-icon icon-bar"></span> 
					<span class="navbar-toggler-icon icon-bar"></span>
				</button>
				&nbsp;&nbsp;&nbsp;{{$.Data.Dashboard.Name}}
                <div class="collapse navbar-collapse justify-content-end">
                    <!--<form class="navbar-form">
                        <div class="input-group no-border">
                            <input type="text" value="" class="form-control" placeholder="Поиск...">
                            <button type="submit" class="btn btn-white btn-round btn-just-icon"><i
                                        class="material-icons notranslate">search</i>
                                <div class="ripple-container"></div>
                            </button>
                        </div>
                    </form>-->
                    <ul class="navbar-nav">
						<!--<li class="nav-item dropdown">
							<a class="nav-link dropdown-toggle" href="javascript:" id="nbModulesDropdown"
                                             role="button" data-toggle="dropdown" aria-haspopup="true"
                                             aria-expanded="false">Модули
							</a>
                			<div class="dropdown-menu" aria-labelledby="nbModulesDropdown"> {{/*$.Data.NavBarModules*/}} </div>
            			</li>
           				<li class="nav-item dropdown">
							<a class="nav-link dropdown-toggle" href="javascript:" id="nbSystemDropdown"
                                             role="button" data-toggle="dropdown" aria-haspopup="true"
                                             aria-expanded="false">Настройки
							</a>
							<div class="dropdown-menu" aria-labelledby="nbSystemDropdown"> {{/*$.Data.NavBarModulesSys*/}} </div>
            			</li>
           				<li class="nav-item">
							<a class="nav-link" href="javascript:fave.FilesManagerDialog();" role="button" aria-expanded="false">Файлы </a>
						</li>-->
                        <li class="nav-item">
                            <a class="nav-link" href="javascript:"> <i class="material-icons notranslate">dashboard</i>
                                <p class="d-lg-none d-md-block"> Stats </p>
                            </a>
                        </li>
                        <li class="nav-item dropdown">
                            <a class="nav-link" href="http://example.com" id="navbarDropdownMenuLink"
                               data-toggle="dropdown" aria-haspopup="true" aria-expanded="false"> <i
                                        class="material-icons notranslate">notifications</i> <span class="notification">5</span>
                                <p class="d-lg-none d-md-block"> Some Actions </p>
                            </a>
                            <div class="dropdown-menu dropdown-menu-right" aria-labelledby="navbarDropdownMenuLink">
								<a class="dropdown-item" href="#">Mike John responded to your email</a> 
								<a class="dropdown-item" href="#">You have 5 new tasks</a> 
								<a class="dropdown-item" href="#">You're now friend with Andrew</a> 
								<a class="dropdown-item" href="#">Another Notification</a> 
								<a class="dropdown-item" href="#">Another One</a>
							</div>
                        </li>
                        <!--<li class="nav-item dropdown">
                            <a class="nav-link" href="javascript:" id="navbarDropdownProfile" data-toggle="dropdown"
                               aria-haspopup="true" aria-expanded="false"> <i class="material-icons notranslate">person</i>
                                <p class="d-lg-none d-md-block"> Account </p>
                            </a>
                            <div class="dropdown-menu dropdown-menu-right" aria-labelledby="navbarDropdownProfile"><a
                                        class="dropdown-item" href="#">Profile</a> <a class="dropdown-item" href="#">Settings</a>
                                <div class="dropdown-divider"></div>
                                <a class="dropdown-item" href="#">Log out</a></div>
                        </li>-->
                    </ul>
					<ul class="navbar-nav">
            			<li class="nav-item dropdown">
							<a class="nav-link dropdown-toggle" href="javascript:" id="nbAccountDropdown"
                                             role="button" data-toggle="dropdown" aria-haspopup="true"
                                             aria-expanded="false">
                    			{{$.Data.UserEmail}}
                			</a>
              				<div class="dropdown-menu dropdown-menu-right" aria-labelledby="nbAccountDropdown">
								<a class="dropdown-item" href="javascript:fave.ModalUserProfile();">Профиль</a>
                    			<div class="dropdown-divider"></div>
                    			<a class="dropdown-item"
                       				href="javascript:fave.ActionLogout('Are you sure want to logout?', '/');">Выход</a>
							</div>
            			</li>
       				 </ul>
                </div>
            </div>
        </nav>
        <!-- End Navbar -->
		<!-- cp start block -->
        <div id="sys-modal-user-settings-placeholder"></div>
        <div id="sys-modal-shop-product-attach-placeholder"></div>
		<div id="sys-modal-files-manager-placeholder"></div>
		<div id="sys-modal-system-message-placeholder"></div>
		<!-- cp end block -->
        <div class="content">
			<!-- cp block -->
			<div class="container-fluid">
				<div class="row">
                    <div class="col-lg-12 col-md-12 col-sm-12">
            			<div class="padd">{{$.Data.Dashboard.Content}}</div>
					</div>
					<!--<div class="col-lg-2 col-md-2 col-sm-2 sidebar-right d-none d-lg-table-cell">
       					<div class="scroll">
     	    				<div class="padd">{{/*$.Data.SidebarRight*/}}</div>
       					</div>
					</div>-->
        		</div>
			<!-- cp block -->
			<!-- cp block -->
			<!--<div class="sidebar sidebar-right d-none d-lg-table-cell">
        		<div class="scroll">
      	    	<div class="padd">{{/*$.Data.SidebarRight*/}}</div>
        	</div>-->
			<!-- cp block -->
    		</div>
        </div>
        <footer class="footer">
            <div class="container-fluid">
                <nav class="float-left">
                    <ul>
                        <li><a href="https://decima24.bitrix24.ru/">
                                Децима
                            </a></li>
                        <li><a href="#">
                                О программе
                            </a></li>
                        <li><a href="#">
                                Помощь
                            </a></li>
                        <li><a href="/cp">
                                Администрирование
                            </a></li>
						<li><a href="#">
                                Лицензии
                            </a></li>
                    </ul>
                </nav>
                <div class="copyright float-right"> &copy;
                    <script>
                        document.write(new Date().getFullYear())
                    </script>
                    , made with <i class="material-icons notranslate">favorite</i> by <a href="https://decima24.bitrix24.ru/"
                                                                             target="_blank">Децима</a> for a
                    better web.
                </div>
            </div>
        </footer>
    </div>
</div>

<!--<div class="fixed-plugin">
    <div class="dropdown show-dropdown">
        <a href="#" data-toggle="dropdown">
            <i class="fa fa-cog fa-2x"> </i>
        </a>
        <ul class="dropdown-menu">
            <li class="header-title">Фильтры сайдбара</li>
            <li class="adjustments-line">
                <a href="javascript:void(0)" class="switch-trigger active-color">
                    <div class="badge-colors ml-auto mr-auto">
                        <span class="badge filter badge-purple" data-color="purple"></span>
                        <span class="badge filter badge-azure" data-color="azure"></span>
                        <span class="badge filter badge-green" data-color="green"></span>
                        <span class="badge filter badge-warning" data-color="orange"></span>
                        <span class="badge filter badge-danger" data-color="danger"></span>
                        <span class="badge filter badge-rose active" data-color="rose"></span>
                    </div>
                    <div class="clearfix"></div>
                </a>
            </li>
            <li class="header-title">Фоновые изображения</li>
            <li class="active">
                <a class="img-holder switch-trigger" href="javascript:void(0)">
                    <img src="{{$.System.PathImageSidebarJpeg1}}" alt="">
                </a>
            </li>
            <li>
                <a class="img-holder switch-trigger" href="javascript:void(0)">
                    <img src="{{$.System.PathImageSidebarJpeg2}}" alt="">
                </a>
            </li>
            <li>
                <a class="img-holder switch-trigger" href="javascript:void(0)">
                    <img src="{{$.System.PathImageSidebarJpeg3}}" alt="">
                </a>
            </li>
            <li>
                <a class="img-holder switch-trigger" href="javascript:void(0)">
                    <img src="{{$.System.PathImageSidebarJpeg4}}" alt="">
                </a>
            </li>
            <!--<li class="button-container">
                <a href="https://www.creative-tim.com/product/material-dashboard" target="_blank"
                   class="btn btn-primary btn-block">Free Download</a>
            </li>
            <li class="button-container">
                <a href="https://demos.creative-tim.com/material-dashboard/docs/2.1/getting-started/introduction.html"
                   target="_blank" class="btn btn-default btn-block">
                    View Documentation
                </a>
            </li>
            <li class="button-container github-star">
                <a class="github-button" href="https://github.com/creativetimofficial/material-dashboard"
                   data-icon="octicon-star" data-size="large" data-show-count="true"
                   aria-label="Star ntkme/github-buttons on GitHub">Star</a>
            </li>
            <li class="header-title">Thank you for 95 shares!</li>
            <li class="button-container text-center">
                <button id="twitter" class="btn btn-round btn-twitter"><i class="fa fa-twitter"></i> &middot; 45
                </button>
                <button id="facebook" class="btn btn-round btn-facebook"><i class="fa fa-facebook-f"></i> &middot; 50
                </button>
                <br>
                <br>
            </li>-->
        </ul>
    </div>
</div>-->
<script src="{{$.System.PathJsJquery}}"></script>
<script src="{{$.System.PathJsPopper}}"></script>
<script src="{{$.System.PathJsBootstrap}}"></script>
{{if or (eq $.System.CpModule "dashboard") (eq $.System.CpModule "index") (eq $.System.CpModule "blog") (eq $.System.CpModule "shop")}}
	{{if or (eq $.System.CpSubModule "add") (eq $.System.CpSubModule "modify")}}
		<script src="{{$.System.PathJsCpWysiwygPell}}"></script>
	{{end}}
{{end}}
{{if eq $.System.CpModule "templates"}}
	<script src="{{$.System.PathJsCpCodeMirror}}"></script>
{{end}}
<script src="{{$.System.PathJsCpScripts}}"></script>
<script src="{{$.System.PathBootstrapMaterialDesign}}"></script>
<script src="{{$.System.PathPerfectScroolbarJs}}"></script>
<script src="{{$.System.PathMomentJs}}"></script>
<script src="{{$.System.PathChartMinJs}}"></script>
<script src="{{$.System.PathSweetalert2Js}}"></script>
<script src="{{$.System.PathValidateJs}}"></script>
<script src="{{$.System.PathBootstrapWizardJs}}"></script>
<script src="{{$.System.PathBootstrapSelectPickerJs}}"></script>
<script src="{{$.System.PathBootstrapDatetimePickerJs}}"></script>
<!--<script src="{{$.System.PathJqueryDataTablesJs}}"></script>-->
<script src="{{$.System.PathBootstrapTagsInputJs}}"></script>
<script src="{{$.System.PathJasnyBootstrapJs}}"></script>
<script src="{{$.System.PathFullcalendarJs}}"></script>
<script src="{{$.System.PathJqueryJvectorMapJs}}"></script>
<script src="{{$.System.PathNouisliderJs}}"></script>
<script src="{{$.System.PathCloudFareJs}}"></script>
<script src="{{$.System.PathArriveJs}}"></script>
<script src="{{$.System.PathChartistJs}}"></script>
<script src="{{$.System.PathBootstrapNotifyJs}}"></script>
<script src="{{$.System.PathMaterialDashboardMinJs}}"></script>
<script src="{{$.System.PathDemoJs}}"></script>
<script>
	$(document).ready(function() {
		  $().ready(function() {
				md.initMinimizeSidebar();

				$sidebar = $('.sidebar');
			
				$sidebar_img_container = $sidebar.find('.sidebar-background');
			
				$full_page = $('.full-page');
			
				$sidebar_responsive = $('body > .navbar-collapse');
			
				window_width = $(window).width();
			
				fixed_plugin_open = $('.sidebar .sidebar-wrapper .nav li.active a p').html();
			
				if (window_width > 767 && fixed_plugin_open == 'Dashboard') {
				  if ($('.fixed-plugin .dropdown').hasClass('show-dropdown')) {
					$('.fixed-plugin .dropdown').addClass('open');
				  }
			
				}
			
				$('.fixed-plugin a').click(function(event) {
				  // Alex if we click on switch, stop propagation of the event, so the dropdown will not be hide, otherwise we set the  section active
				  if ($(this).hasClass('switch-trigger')) {
					if (event.stopPropagation) {
					  event.stopPropagation();
					} else if (window.event) {
					  window.event.cancelBubble = true;
					}
				  }
				});
			
				$('.fixed-plugin .active-color span').click(function() {
				  $full_page_background = $('.full-page-background');
			
				  $(this).siblings().removeClass('active');
				  $(this).addClass('active');
			
				  var new_color = $(this).data('color');
			
				  if ($sidebar.length != 0) {
					$sidebar.attr('data-color', new_color);
				  }
			
				  if ($full_page.length != 0) {
					$full_page.attr('filter-color', new_color);
				  }
			
				  if ($sidebar_responsive.length != 0) {
					$sidebar_responsive.attr('data-color', new_color);
				  }
				});
			
				$('.fixed-plugin .background-color .badge').click(function() {
				  $(this).siblings().removeClass('active');
				  $(this).addClass('active');
			
				  var new_color = $(this).data('background-color');
			
				  if ($sidebar.length != 0) {
					$sidebar.attr('data-background-color', new_color);
				  }
				});
			
				$('.fixed-plugin .img-holder').click(function() {
				  $full_page_background = $('.full-page-background');
			
				  $(this).parent('li').siblings().removeClass('active');
				  $(this).parent('li').addClass('active');
			
			
				  var new_image = $(this).find("img").attr('src');
			
				  if ($sidebar_img_container.length != 0 && $('.switch-sidebar-image input:checked').length != 0) {
					$sidebar_img_container.fadeOut('fast', function() {
					  $sidebar_img_container.css('background-image', 'url("' + new_image + '")');
					  $sidebar_img_container.fadeIn('fast');
					});
				  }
			
				  if ($full_page_background.length != 0 && $('.switch-sidebar-image input:checked').length != 0) {
					var new_image_full_page = $('.fixed-plugin li.active .img-holder').find('img').data('src');
			
					$full_page_background.fadeOut('fast', function() {
					  $full_page_background.css('background-image', 'url("' + new_image_full_page + '")');
					  $full_page_background.fadeIn('fast');
					});
				  }
			
				  if ($('.switch-sidebar-image input:checked').length == 0) {
					var new_image = $('.fixed-plugin li.active .img-holder').find("img").attr('src');
					var new_image_full_page = $('.fixed-plugin li.active .img-holder').find('img').data('src');
			
					$sidebar_img_container.css('background-image', 'url("' + new_image + '")');
					$full_page_background.css('background-image', 'url("' + new_image_full_page + '")');
				  }
			
				  if ($sidebar_responsive.length != 0) {
					$sidebar_responsive.css('background-image', 'url("' + new_image + '")');
				  }
				});
			
				$('.switch-sidebar-image input').change(function() {
				  $full_page_background = $('.full-page-background');
			
				  $input = $(this);
			
				  if ($input.is(':checked')) {
					if ($sidebar_img_container.length != 0) {
					  $sidebar_img_container.fadeIn('fast');
					  $sidebar.attr('data-image', '#');
					}
			
					if ($full_page_background.length != 0) {
					  $full_page_background.fadeIn('fast');
					  $full_page.attr('data-image', '#');
					}
			
					background_image = true;
				  } else {
					if ($sidebar_img_container.length != 0) {
					  $sidebar.removeAttr('data-image');
					  $sidebar_img_container.fadeOut('fast');
					}
			
					if ($full_page_background.length != 0) {
					  $full_page.removeAttr('data-image', '#');
					  $full_page_background.fadeOut('fast');
					}
			
					background_image = false;
				  }
				});
			
				$('.switch-sidebar-mini input').change(function() {
				  $body = $('body');
			
				  $input = $(this);
			
				  if (md.misc.sidebar_mini_active == true) {
					$('body').removeClass('sidebar-mini');
					md.misc.sidebar_mini_active = false;
			
					$('.sidebar .sidebar-wrapper, .main-panel').perfectScrollbar();
			
				  } else {
			
					$('.sidebar .sidebar-wrapper, .main-panel').perfectScrollbar('destroy');
			
					setTimeout(function() {
					  $('body').addClass('sidebar-mini');
			
					  md.misc.sidebar_mini_active = true;
					}, 300);
				  }
			
				  // we simulate the window Resize so the charts will get updated in realtime.
				  var simulateWindowResize = setInterval(function() {
					window.dispatchEvent(new Event('resize'));
				  }, 180);
			
				  // we stop the simulation of Window Resize after the animations are completed
				  setTimeout(function() {
					clearInterval(simulateWindowResize);
				  }, 1000);
			
				});
		  });
	});
</script>
<!--<script>
	function hidestatus(e){
		if($(e.target).attr("href") != void 0) {
			console.log(e.target);
			window.status=''
			return true
		}
	}

	if (document.layers) {
		document.captureEvents(Event.MOUSEOVER | Event.MOUSEOUT)
	}

	document.onmouseover=hidestatus
	document.onmouseout=hidestatus
</script>-->

</body>
</html>`)
