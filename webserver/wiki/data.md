[...оглавление](./main.md)


######Переменные для шаблона ($.Data)

#####Http
| Название переменной | Тип переменной | Описание |
| :--- | :--- | :--- |
|{{$.Data.RequestGET}}	|String	|Just GET parameters from URL|
|{{$.Data.RequestURI}}	|String	|Full URL string with GET parameters|
|{{$.Data.RequestURL}}	|String	|Full URL without GET parameters|


#####Сommon
| Название переменной | Тип переменной | Описание |
| :--- | :--- | :--- |
|{{$.Data.DateTimeFormat}}	|String	|Function (format string), returns formated current date and time|
|{{$.Data.DateTimeUnix}}	|Integer	|Returns current unix timestamp|
|{{$.Data.Module}}	|String	|404, index, blog, blog-category, blog-post|

#####\{\{$.Data.Page\}\}
| Название переменной | Тип переменной | Описание |
| :--- | :--- | :--- |
|.Id	|Integer	|Current page ID, default: 0|
|.User	|*User	|Current page user, default: nil|
|.Name	|String	|Current page name, default: ""|
|.Alias	|String	|Current page alias, default: ""|
|.Content	|HTML	|Current page content, default: ""|
|.MetaTitle	|String	|Current page meta title, default: ""|
|.MetaKeywords	|String	|Current page meta keywords, default: ""|
|.MetaDescription	|String	|Current page meta description, default: ""|
|.DateTimeUnix	|Integer	|Current page create unix timestamp, default: 0|
|.DateTimeFormat	|String	|Current page create formated date and time, function (format string), default: ""|
|.Active	|Boolean	|Current page active status, default: false|


#####\{\{$.Data.Blog\}\}
| Название переменной | Тип переменной | Описание |
| :--- | :--- | :--- |
|.Category|	*BlogCategory	|Current blog category, default: nil
|.Post	|*BlogPost	|Current blog post, default: nil|
|.HavePosts	|Boolean	|Returns true, if blog main page or selected category have one or more posts|
|.Posts	|[]*BlogPost	|Returns array of blog posts of main page or selected category|
|.PostsCount|	Integer	|Blog posts count of main page or selected category|
|.PostsPerPage|	Integer	|How many posts to display at page, value from control panel|
|.PostsMaxPage|	Integer	|Calculated value, max page, based on PostsPerPage|
|.PostsCurrPage|	Integer	|Returns current page, based on URL|
|.Pagination	|[]*BlogPagination	|For pagination building|
|.PaginationPrev|	*BlogPagination	|One pagination item for previous page|
|.PaginationNext|	*BlogPagination	|One pagination item for next page|
|.Categories	|[]*BlogCategory|	Function (mlvl int), returns array of categories to some level|

#####\{\{$.Data.Blog.Category\}\}
| Название переменной | Тип переменной | Описание |
| :--- | :--- | :--- |
|.Id	|Integer	|Category ID, default: 0|
|.User	|*User	|Category user, category owner, default: nil|
|.Name	|String	|Category name, default: ""|
|.Alias	|String	|Category alias, default: ""|
|.Left	|Integer	|Category left value, default: 0|
|.Right	|Integer	|Category left value, default: 0|
|.Permalink	|String|	Category link without host name, default: ""|
|.Level|	Integer|	Category level in hierarchy (depth), default: 0|

#####\{\{$.Data.Blog.Post\}\}
| Название переменной | Тип переменной | Описание |
| :--- | :--- | :--- |
|.Id	|Integer	|Post ID, default: 0|
|.User	|*User	|Post user, post owner, default: nil|
|.Name	|String	|Post name, default: ""|
|.Alias	|String	|Post alias, default: ""|
|.Briefly	|HTML	|Post short content, default: ""|
|.Content	|HTML	|Post main content, default: ""|
|.DateTimeUnix	|Integer	|Post create unix timestamp, default: 0|
|.DateTimeFormat	|String	|Post create formated date and time, function (format string), default: ""|
|.Active	|Boolean	|Post active status, default: false|
|.Permalink	|String	|Post link without host name, default: ""|

#####*User
| Название переменной | Тип переменной | Описание |
| :--- | :--- | :--- |
|.Id	|Integer	|User ID, default: 0|
|.FirstName	|String	|User first name, default: ""|
|.LastName	|String	|User last name, default: ""|
|.Email	|String	|User email, default: ""|
|.IsAdmin	|Boolean	|Returns true if user is admin, default: false|
|.IsActive	|Boolean	|Returns true if user is active, default: false|

#####*BlogPagination
| Название переменной | Тип переменной | Описание |
| :--- | :--- | :--- |
|.Num	|Integer	|Page number, default: 0|
|.Link	|String	|Full page link, default: ""|
|.Current	|Boolean	|Returns true if this is current page, default: false|
|.Dots	|Boolean	|Returns true if this is dots, not page button, default: false|