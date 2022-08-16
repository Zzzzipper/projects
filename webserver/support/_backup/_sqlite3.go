package support

import (
	"context"
	"fmt"
	"server/engine/sqlw"
	"server/engine/utils"
)

func InitDataBase(ctx context.Context, db *sqlw.DB) error {
	// Start transaction
	tx, err := db.Begin(ctx)
	if err != nil {
		return err
	}

	// Table: blog_cats
	if _, err = tx.Exec(
		ctx,
		`CREATE TABLE IF NOT EXISTS blog_cats (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, -- AI
			user INTEGER NOT NULL, -- User id
			name varchar(255) NOT NULL, -- Category name
			alias varchar(255) NOT NULL, -- Category alias
			lft INTEGER NOT NULL, -- For nested set model
			rgt INTEGER NOT NULL, -- For nested set model
			FOREIGN KEY(user) REFERENCES users(id)
		);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("blog_cats: %s", err.Error())
	}

	// Table: blog_cat_post_rel
	if _, err = tx.Exec(
		ctx,
		`CREATE TABLE IF NOT EXISTS blog_cat_post_rel (
			post_id INTEGER NOT NULL, -- 'Post id'
			category_id INTEGER NOT NULL, -- 'Category id'
			FOREIGN KEY(post_id) REFERENCES blog_posts(id),
			FOREIGN KEY(category_id) REFERENCES blog_cats(id)
		);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("blog_cat_post_rel: %s", err.Error())
	}

	// Table: blog_posts
	if _, err = tx.Exec(
		ctx,
		`CREATE TABLE IF NOT EXISTS blog_posts (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,-- 'AI'
			user INTEGER NOT NULL, -- 'User id'
			name varchar(255) NOT NULL, -- 'Post name'
			alias varchar(255) NOT NULL, -- 'Post alias'
			category INTEGER NOT NULL,
			briefly text NOT NULL, -- 'Post brief content'
			content text NOT NULL, -- 'Post content'
			datetime datetime NOT NULL, -- 'Creation date/time'
			active INTEGER NOT NULL, -- 'Is active post or not'
			FOREIGN KEY(user) REFERENCES users(id),
			FOREIGN KEY(category) REFERENCES blog_cats(id)
		);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("blog_posts: %s", err.Error())
	}

	// Table: notify_mail
	if _, err = tx.Exec(
		ctx,
		`CREATE TABLE IF NOT EXISTS notify_mail (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, -- 'AI'
			email varchar(255) NOT NULL, -- 'Email address'
			subject varchar(800) NOT NULL, -- 'Email subject'
			message text NOT NULL, -- 'Email body'
			error text NOT NULL, -- 'Send error message'
			datetime datetime NOT NULL, -- 'Creation date/time'
			status INTEGER NOT NULL -- 'Sending status'
		);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("notify_mail: %s", err.Error())
	}

	// Table: pages
	if _, err = tx.Exec(
		ctx,
		`CREATE TABLE IF NOT EXISTS pages (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, -- 'AI'
			user INTEGER NOT NULL, -- 'User id'
			template varchar(255) NOT NULL DEFAULT 'page', -- 'Template'
			name varchar(255) NOT NULL, -- 'Page name'
			alias varchar(255) NOT NULL, -- 'Page url part'
			content text NOT NULL, -- 'Page content'
			meta_title varchar(255) NOT NULL DEFAULT '', -- 'Page meta title'
			meta_keywords varchar(255) NOT NULL DEFAULT '', -- 'Page meta keywords'
			meta_description varchar(510) NOT NULL DEFAULT '', -- 'Page meta description'
			datetime datetime NOT NULL, -- 'Creation date/time'
			active INTEGER NOT NULL, -- 'Is active page or not'
			FOREIGN KEY(user) REFERENCES users(id)
		);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("pages: %s", err.Error())
	}

	// Table: settings
	if _, err = tx.Exec(
		ctx,
		`CREATE TABLE IF NOT EXISTS settings (
			name varchar(255) NOT NULL, -- 'Setting name'
			value text NOT NULL -- 'Setting value'
		);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("settings: %s", err.Error())
	}

	// Table: shop_cat_product_rel
	if _, err = tx.Exec(
		ctx,
		`CREATE TABLE IF NOT EXISTS shop_cat_product_rel (
			product_id INTEGER NOT NULL, -- 'Product id'
			category_id INTEGER NOT NULL, -- 'Category id'
			FOREIGN KEY(product_id) REFERENCES shop_products(id),
			FOREIGN KEY(category_id) REFERENCES shop_cats(id)
		);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("shop_cat_product_rel: %s", err.Error())
	}

	// Table: shop_cats
	if _, err = tx.Exec(
		ctx,
		`CREATE TABLE IF NOT EXISTS shop_cats (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, -- 'AI'
			user INTEGER NOT NULL, -- 'User id'
			name varchar(255) NOT NULL, -- 'Category name'
			alias varchar(255) NOT NULL, -- 'Category alias'
			lft INTEGER NOT NULL, -- 'For nested set model'
			rgt INTEGER NOT NULL, -- 'For nested set model'
			FOREIGN KEY(user) REFERENCES user(id)
		);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("shop_cats: %s", err.Error())
	}

	// Table: shop_currencies
	if _, err = tx.Exec(
		ctx,
		`CREATE TABLE IF NOT EXISTS shop_currencies (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, -- 'AI'
			name varchar(255) NOT NULL, -- 'Currency name'
			coefficient REAL NOT NULL DEFAULT '1.0000', -- 'Currency coefficient'
			code varchar(10) NOT NULL, -- 'Currency code'
			symbol varchar(5) NOT NULL -- 'Currency symbol'
		);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("shop_currencies: %s", err.Error())
	}

	// Table: shop_filter_product_values
	if _, err = tx.Exec(
		ctx,
		`CREATE TABLE IF NOT EXISTS shop_filter_product_values (
			product_id INTEGER NOT NULL, -- 'Product id'
			filter_value_id INTEGER NOT NULL, -- 'Filter value id'
			FOREIGN KEY(product_id) REFERENCES shop_products(id)
		);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("shop_filter_product_values: %s", err.Error())
	}

	// Table: shop_filters
	if _, err = tx.Exec(
		ctx,
		`CREATE TABLE IF NOT EXISTS shop_filters (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, -- 'AI'
			name varchar(255) NOT NULL, -- 'Filter name in CP'
			filter varchar(255) NOT NULL -- 'Filter name in site'
		);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("shop_filters: %s", err.Error())
	}

	// Table: shop_filters_values
	if _, err = tx.Exec(
		ctx,
		`CREATE TABLE IF NOT EXISTS shop_filters_values (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, -- 'AI'
			filter_id INTEGER NOT NULL, -- 'Filter id'
			name varchar(255) NOT NULL, -- 'Value name'
			FOREIGN KEY(filter_id) REFERENCES shop_filters(id)
		);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("shop_filters_values: %s", err.Error())
	}

	// Table: shop_order_products
	if _, err = tx.Exec(
		ctx,
		`CREATE TABLE IF NOT EXISTS shop_order_products (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, -- 'AI'
			order_id INTEGER NOT NULL, -- 'Order ID'
			product_id INTEGER NOT NULL, -- 'Product ID'
			price float(8,2) NOT NULL, -- 'Product price'
			quantity INTEGER NOT NULL, -- 'Quantity'
			FOREIGN KEY(order_id) REFERENCES shop_orders(id),
			FOREIGN KEY(product_id) REFERENCES shop_products(id)
		);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("shop_order_products: %s", err.Error())
	}

	// Table: shop_orders
	if _, err = tx.Exec(
		ctx,
		`CREATE TABLE IF NOT EXISTS shop_orders (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, -- 'AI'
			create_datetime datetime NOT NULL, -- 'Create date/time'
			update_datetime datetime NOT NULL, -- 'Update date/time'
			currency_id INTEGER NOT NULL, -- 'Currency ID'
			currency_name varchar(255) NOT NULL, -- 'Currency name'
			currency_coefficient REAL NOT NULL DEFAULT '1.0000', -- 'Currency coefficient'
			currency_code varchar(10) NOT NULL, -- 'Currency code'
			currency_symbol varchar(5) NOT NULL, -- 'Currency symbol'
			client_last_name varchar(64) NOT NULL, -- 'Client last name'
			client_first_name varchar(64) NOT NULL, -- 'Client first name'
			client_middle_name varchar(64) NOT NULL DEFAULT '', -- 'Client middle name'
			client_phone varchar(20) NOT NULL DEFAULT '', -- 'Client phone'
			client_email varchar(64) NOT NULL, -- 'Client email'
			client_delivery_comment text NOT NULL, -- 'Client delivery comment'
			client_order_comment text NOT NULL, -- 'Client order comment'
			status INTEGER NOT NULL, -- 'new/confirmed/inprogress/canceled/completed'
			FOREIGN KEY(currency_id) REFERENCES shop_currencies(id)
		);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("shop_orders: %s", err.Error())
	}

	// Table: shop_product_images
	if _, err = tx.Exec(
		ctx,
		`CREATE TABLE IF NOT EXISTS shop_product_images (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
			product_id INTEGER NOT NULL,
			filename varchar(255) NOT NULL,
			ord INTEGER NOT NULL DEFAULT '0',
			FOREIGN KEY(product_id) REFERENCES shop_products(id)
		);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("shop_product_images: %s", err.Error())
	}

	// Table: shop_products
	if _, err = tx.Exec(
		ctx,
		`CREATE TABLE IF NOT EXISTS shop_products (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, -- 'AI'
			parent_id INTEGER DEFAULT NULL,
			user INTEGER NOT NULL, -- 'User id'
			currency INTEGER NOT NULL, -- 'Currency id'
			price float(8,2) NOT NULL, -- 'Product price'
			price_old float(8,2) NOT NULL DEFAULT '0.00',
			price_promo float(8,2) NOT NULL DEFAULT '0.00',
			gname varchar(255) NOT NULL,
			name varchar(255) NOT NULL, -- 'Product name'
			alias varchar(255) NOT NULL, -- 'Product alias'
			vendor varchar(255) NOT NULL,
			quantity INTEGER NOT NULL,
			category INTEGER NOT NULL,
			briefly text NOT NULL, -- 'Product brief content'
			content text NOT NULL, -- 'Product content'
			datetime datetime NOT NULL, -- 'Creation date/time'
			active int(1) NOT NULL, -- 'Is active product or not'
			custom1 varchar(2048) NOT NULL DEFAULT '',
			custom2 varchar(2048) NOT NULL DEFAULT '',
			FOREIGN KEY(user) REFERENCES users(id),
			FOREIGN KEY(currency) REFERENCES shop_currencies(id),
			FOREIGN KEY(category) REFERENCES shop_cats(id),
			FOREIGN KEY(parent_id) REFERENCES shop_products(id)
		);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("shop_products: %s", err.Error())
	}

	// Table: users
	if _, err = tx.Exec(
		ctx,
		`CREATE TABLE IF NOT EXISTS users (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, -- 'AI'
			first_name varchar(64) NOT NULL DEFAULT '', -- 'User first name'
			last_name varchar(64) NOT NULL DEFAULT '', -- 'User last name'
			email varchar(64) NOT NULL, -- 'User email'
			password varchar(32) NOT NULL, -- 'User password (MD5)'
			admin int(1) NOT NULL, -- 'Is admin user or not'
			active int(1) NOT NULL, -- 'Is active user or not'
			address varchar(64) DEFAULT '',
			port INTEGER DEFAULT 0,
			signature VARBINARY(80) NOT NULL DEFAULT ''  -- 'AES signature'
		);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("users: %s", err.Error())
	}

	//Table: dashboards
	if _, err = tx.Exec(
		ctx,
		`CREATE TABLE IF NOT EXISTS dashboards (
			id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, -- 'AI'
			user INTEGR NOT NULL, -- 'User id'
			template varchar(255) NOT NULL DEFAULT 'page', -- 'Template'
			name varchar(255) NOT NULL, -- 'Page name'
			alias varchar(255) NOT NULL,  -- 'Page url part'
			content text NOT NULL, -- 'Page content'
			meta_title varchar(255) NOT NULL DEFAULT '', -- 'Page meta title'
			meta_keywords varchar(255) NOT NULL DEFAULT '', -- 'Page meta keywords'
			meta_description varchar(510) NOT NULL DEFAULT '', -- 'Page meta description'
			datetime datetime NOT NULL, -- 'Creation date/time'
			active INTEGER NOT NULL, --'Is active page or not'
			FOREIGN KEY(user) REFERENCES users(id)
		  );`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("dashboards: %s", err.Error())
	}

	// Demo datas
	if _, err = tx.Exec(
		ctx,
		`INSERT INTO blog_cats (id, user, name, alias, lft, rgt)
			VALUES
		(1, 1, 'ROOT', 'ROOT', 1, 24),
		(2, 1, 'Health and food', 'health-and-food', 2, 15),
		(3, 1, 'News', 'news', 16, 21),
		(4, 1, 'Hobby', 'hobby', 22, 23),
		(5, 1, 'Juices', 'juices', 3, 8),
		(6, 1, 'Nutrition', 'nutrition', 9, 14),
		(7, 1, 'Natural', 'natural', 4, 5),
		(8, 1, 'For kids', 'for-kids', 6, 7),
		(9, 1, 'For all', 'for-all', 10, 11),
		(10, 1, 'For athletes', 'for-athletes', 12, 13),
		(11, 1, 'Computers and technology', 'computers-and-technology', 17, 18),
		(12, 1, 'Film industry', 'film-industry', 19, 20);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("INSERT INTO blog_cats: %s", err.Error())
	}
	if _, err = tx.Exec(
		ctx,
		`INSERT INTO blog_cat_post_rel (post_id, category_id) VALUES (1, 9), (2, 12), (3, 8);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("INSERT INTO blog_cat_post_rel: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`INSERT INTO blog_posts (
			id,
			user,
			name,
			alias,
			category,
			briefly,
			content,
			datetime,
			active
		) VALUES (
			?,
			?,
			?,
			?,
			?,
			?,
			?,
			?,
			?
		)				
		;`,
		1,
		1,
		"Why should we eat wholesome food?",
		"why-should-we-eat-wholesome-food",
		9,
		"<p>Arcu ac tortor dignissim convallis aenean et tortor. Vitae auctor eu augue ut lectus arcu. Ac turpis egestas integer eget aliquet nibh praesent. Interdum velit euismod in pellentesque massa placerat duis. Vestibulum rhoncus est pellentesque elit ullamcorper dignissim cras tincidunt. Nisl rhoncus mattis rhoncus urna neque viverra justo. Odio ut enim blandit volutpat. Ac auctor augue mauris augue neque gravida. Ut lectus arcu bibendum at varius vel. Porttitor leo a diam sollicitudin tempor id eu nisl nunc. Dolor sit amet consectetur adipiscing elit duis tristique. Semper quis lectus nulla at volutpat diam ut. Sapien eget mi proin sed.</p>",
		"<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Feugiat in ante metus dictum at tempor commodo ullamcorper a. Et malesuada fames ac turpis egestas sed tempus urna et. Euismod elementum nisi quis eleifend. Nisi porta lorem mollis aliquam ut porttitor. Ac turpis egestas maecenas pharetra convallis posuere. Nunc non blandit massa enim nec dui. Commodo elit at imperdiet dui accumsan sit amet nulla. Viverra accumsan in nisl nisi scelerisque. Dui nunc mattis enim ut tellus. Molestie ac feugiat sed lectus vestibulum mattis ullamcorper. Faucibus ornare suspendisse sed nisi lacus. Nulla facilisi morbi tempus iaculis. Ut eu sem integer vitae justo eget magna fermentum iaculis. Ullamcorper sit amet risus nullam eget felis eget nunc. Volutpat sed cras ornare arcu dui vivamus. Eget magna fermentum iaculis eu non diam.</p><p>Arcu ac tortor dignissim convallis aenean et tortor. Vitae auctor eu augue ut lectus arcu. Ac turpis egestas integer eget aliquet nibh praesent. Interdum velit euismod in pellentesque massa placerat duis. Vestibulum rhoncus est pellentesque elit ullamcorper dignissim cras tincidunt. Nisl rhoncus mattis rhoncus urna neque viverra justo. Odio ut enim blandit volutpat. Ac auctor augue mauris augue neque gravida. Ut lectus arcu bibendum at varius vel. Porttitor leo a diam sollicitudin tempor id eu nisl nunc. Dolor sit amet consectetur adipiscing elit duis tristique. Semper quis lectus nulla at volutpat diam ut. Sapien eget mi proin sed.</p>",
		utils.UnixTimestampToMySqlDateTime(utils.GetCurrentUnixTimestamp()),
		1,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("INSERT INTO blog_posts: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`INSERT INTO blog_posts(
			id,
			user,
			name,
			alias,
			category,
			briefly,
			content,
			datetime,
			active
		) VALUES (
			?,
			?,
			?,
			?,
			?,
			?,
			?,
			?,
			?
		)
		;`,
		2,
		1,
		"Latest top space movies",
		"latest-top-space-movies",
		12,
		"<p>Arcu ac tortor dignissim convallis aenean et tortor. Vitae auctor eu augue ut lectus arcu. Ac turpis egestas integer eget aliquet nibh praesent. Interdum velit euismod in pellentesque massa placerat duis. Vestibulum rhoncus est pellentesque elit ullamcorper dignissim cras tincidunt. Nisl rhoncus mattis rhoncus urna neque viverra justo. Odio ut enim blandit volutpat. Ac auctor augue mauris augue neque gravida. Ut lectus arcu bibendum at varius vel. Porttitor leo a diam sollicitudin tempor id eu nisl nunc. Dolor sit amet consectetur adipiscing elit duis tristique. Semper quis lectus nulla at volutpat diam ut. Sapien eget mi proin sed.</p>",
		"<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Feugiat in ante metus dictum at tempor commodo ullamcorper a. Et malesuada fames ac turpis egestas sed tempus urna et. Euismod elementum nisi quis eleifend. Nisi porta lorem mollis aliquam ut porttitor. Ac turpis egestas maecenas pharetra convallis posuere. Nunc non blandit massa enim nec dui. Commodo elit at imperdiet dui accumsan sit amet nulla. Viverra accumsan in nisl nisi scelerisque. Dui nunc mattis enim ut tellus. Molestie ac feugiat sed lectus vestibulum mattis ullamcorper. Faucibus ornare suspendisse sed nisi lacus. Nulla facilisi morbi tempus iaculis. Ut eu sem integer vitae justo eget magna fermentum iaculis. Ullamcorper sit amet risus nullam eget felis eget nunc. Volutpat sed cras ornare arcu dui vivamus. Eget magna fermentum iaculis eu non diam.</p><p>Arcu ac tortor dignissim convallis aenean et tortor. Vitae auctor eu augue ut lectus arcu. Ac turpis egestas integer eget aliquet nibh praesent. Interdum velit euismod in pellentesque massa placerat duis. Vestibulum rhoncus est pellentesque elit ullamcorper dignissim cras tincidunt. Nisl rhoncus mattis rhoncus urna neque viverra justo. Odio ut enim blandit volutpat. Ac auctor augue mauris augue neque gravida. Ut lectus arcu bibendum at varius vel. Porttitor leo a diam sollicitudin tempor id eu nisl nunc. Dolor sit amet consectetur adipiscing elit duis tristique. Semper quis lectus nulla at volutpat diam ut. Sapien eget mi proin sed.</p>",
		utils.UnixTimestampToMySqlDateTime(utils.GetCurrentUnixTimestamp()),
		1,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("INSERT INTO blog_posts 1: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`INSERT INTO blog_posts (
			id,
			user,
			name,
			alias,
			category,
			briefly,
			content,
			datetime,
			active
		) VALUES (
			?,
			?,
			?,
			?,
			?,
			?,
			?,
			?,
			?
		);`,
		3,
		1,
		"The best juices for a child",
		"the-best-juices-for-a-child",
		8,
		"<p>Arcu ac tortor dignissim convallis aenean et tortor. Vitae auctor eu augue ut lectus arcu. Ac turpis egestas integer eget aliquet nibh praesent. Interdum velit euismod in pellentesque massa placerat duis. Vestibulum rhoncus est pellentesque elit ullamcorper dignissim cras tincidunt. Nisl rhoncus mattis rhoncus urna neque viverra justo. Odio ut enim blandit volutpat. Ac auctor augue mauris augue neque gravida. Ut lectus arcu bibendum at varius vel. Porttitor leo a diam sollicitudin tempor id eu nisl nunc. Dolor sit amet consectetur adipiscing elit duis tristique. Semper quis lectus nulla at volutpat diam ut. Sapien eget mi proin sed.</p>",
		"<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Feugiat in ante metus dictum at tempor commodo ullamcorper a. Et malesuada fames ac turpis egestas sed tempus urna et. Euismod elementum nisi quis eleifend. Nisi porta lorem mollis aliquam ut porttitor. Ac turpis egestas maecenas pharetra convallis posuere. Nunc non blandit massa enim nec dui. Commodo elit at imperdiet dui accumsan sit amet nulla. Viverra accumsan in nisl nisi scelerisque. Dui nunc mattis enim ut tellus. Molestie ac feugiat sed lectus vestibulum mattis ullamcorper. Faucibus ornare suspendisse sed nisi lacus. Nulla facilisi morbi tempus iaculis. Ut eu sem integer vitae justo eget magna fermentum iaculis. Ullamcorper sit amet risus nullam eget felis eget nunc. Volutpat sed cras ornare arcu dui vivamus. Eget magna fermentum iaculis eu non diam.</p><p>Arcu ac tortor dignissim convallis aenean et tortor. Vitae auctor eu augue ut lectus arcu. Ac turpis egestas integer eget aliquet nibh praesent. Interdum velit euismod in pellentesque massa placerat duis. Vestibulum rhoncus est pellentesque elit ullamcorper dignissim cras tincidunt. Nisl rhoncus mattis rhoncus urna neque viverra justo. Odio ut enim blandit volutpat. Ac auctor augue mauris augue neque gravida. Ut lectus arcu bibendum at varius vel. Porttitor leo a diam sollicitudin tempor id eu nisl nunc. Dolor sit amet consectetur adipiscing elit duis tristique. Semper quis lectus nulla at volutpat diam ut. Sapien eget mi proin sed.</p>",
		utils.UnixTimestampToMySqlDateTime(utils.GetCurrentUnixTimestamp()),
		1,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("INSERT INTO blog_posts 2: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`INSERT INTO pages (
			id,
			user,
			template,
			name,
			alias,
			content,
			datetime,
			active
		) VALUES (
			?,
			?,
			?,
			?,
			?,
			?,
			?,
			?
		);`,
		1,
		1,
		"index",
		"Home",
		"/",
		"<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Feugiat in ante metus dictum at tempor commodo ullamcorper a. Et malesuada fames ac turpis egestas sed tempus urna et. Euismod elementum nisi quis eleifend. Nisi porta lorem mollis aliquam ut porttitor. Ac turpis egestas maecenas pharetra convallis posuere. Nunc non blandit massa enim nec dui. Commodo elit at imperdiet dui accumsan sit amet nulla. Viverra accumsan in nisl nisi scelerisque. Dui nunc mattis enim ut tellus. Molestie ac feugiat sed lectus vestibulum mattis ullamcorper. Faucibus ornare suspendisse sed nisi lacus. Nulla facilisi morbi tempus iaculis. Ut eu sem integer vitae justo eget magna fermentum iaculis. Ullamcorper sit amet risus nullam eget felis eget nunc. Volutpat sed cras ornare arcu dui vivamus. Eget magna fermentum iaculis eu non diam.</p><p>Arcu ac tortor dignissim convallis aenean et tortor. Vitae auctor eu augue ut lectus arcu. Ac turpis egestas integer eget aliquet nibh praesent. Interdum velit euismod in pellentesque massa placerat duis. Vestibulum rhoncus est pellentesque elit ullamcorper dignissim cras tincidunt. Nisl rhoncus mattis rhoncus urna neque viverra justo. Odio ut enim blandit volutpat. Ac auctor augue mauris augue neque gravida. Ut lectus arcu bibendum at varius vel. Porttitor leo a diam sollicitudin tempor id eu nisl nunc. Dolor sit amet consectetur adipiscing elit duis tristique. Semper quis lectus nulla at volutpat diam ut. Sapien eget mi proin sed.</p><p>Ante metus dictum at tempor commodo ullamcorper a. Facilisis mauris sit amet massa vitae. Enim neque volutpat ac tincidunt vitae. Tempus quam pellentesque nec nam aliquam sem. Mollis aliquam ut porttitor leo a diam sollicitudin. Nunc pulvinar sapien et ligula ullamcorper. Dignissim suspendisse in est ante in nibh mauris. Eget egestas purus viverra accumsan in. Vitae tempus quam pellentesque nec nam aliquam sem et. Sodales ut etiam sit amet nisl. Aliquet risus feugiat in ante. Rhoncus urna neque viverra justo nec ultrices dui sapien. Sit amet aliquam id diam maecenas ultricies. Sed odio morbi quis commodo odio aenean sed adipiscing diam.</p>",
		utils.UnixTimestampToMySqlDateTime(utils.GetCurrentUnixTimestamp()),
		1,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("INSERT INTO pages 1: %s", err.Error())
	}
	if _, err = tx.Exec(
		ctx,
		`INSERT INTO pages (
			id,
			user,
			template,
			name,
			alias,
			content,
			datetime,
			active
		) VALUES (
			?,
			?,
			?,
			?,
			?,
			?,
			?,
			?
		);`,
		2,
		1,
		"page",
		"Another",
		"/another/",
		"<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Feugiat in ante metus dictum at tempor commodo ullamcorper a. Et malesuada fames ac turpis egestas sed tempus urna et. Euismod elementum nisi quis eleifend. Nisi porta lorem mollis aliquam ut porttitor. Ac turpis egestas maecenas pharetra convallis posuere. Nunc non blandit massa enim nec dui. Commodo elit at imperdiet dui accumsan sit amet nulla. Viverra accumsan in nisl nisi scelerisque. Dui nunc mattis enim ut tellus. Molestie ac feugiat sed lectus vestibulum mattis ullamcorper. Faucibus ornare suspendisse sed nisi lacus. Nulla facilisi morbi tempus iaculis. Ut eu sem integer vitae justo eget magna fermentum iaculis. Ullamcorper sit amet risus nullam eget felis eget nunc. Volutpat sed cras ornare arcu dui vivamus. Eget magna fermentum iaculis eu non diam.</p><p>Arcu ac tortor dignissim convallis aenean et tortor. Vitae auctor eu augue ut lectus arcu. Ac turpis egestas integer eget aliquet nibh praesent. Interdum velit euismod in pellentesque massa placerat duis. Vestibulum rhoncus est pellentesque elit ullamcorper dignissim cras tincidunt. Nisl rhoncus mattis rhoncus urna neque viverra justo. Odio ut enim blandit volutpat. Ac auctor augue mauris augue neque gravida. Ut lectus arcu bibendum at varius vel. Porttitor leo a diam sollicitudin tempor id eu nisl nunc. Dolor sit amet consectetur adipiscing elit duis tristique. Semper quis lectus nulla at volutpat diam ut. Sapien eget mi proin sed.</p><p>Ante metus dictum at tempor commodo ullamcorper a. Facilisis mauris sit amet massa vitae. Enim neque volutpat ac tincidunt vitae. Tempus quam pellentesque nec nam aliquam sem. Mollis aliquam ut porttitor leo a diam sollicitudin. Nunc pulvinar sapien et ligula ullamcorper. Dignissim suspendisse in est ante in nibh mauris. Eget egestas purus viverra accumsan in. Vitae tempus quam pellentesque nec nam aliquam sem et. Sodales ut etiam sit amet nisl. Aliquet risus feugiat in ante. Rhoncus urna neque viverra justo nec ultrices dui sapien. Sit amet aliquam id diam maecenas ultricies. Sed odio morbi quis commodo odio aenean sed adipiscing diam.</p>",
		utils.UnixTimestampToMySqlDateTime(utils.GetCurrentUnixTimestamp()),
		1,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("INSERT INTO pages 2: %s", err.Error())
	}
	if _, err = tx.Exec(
		ctx,
		`INSERT INTO pages (
			id,
			user,
			template,
			name,
			alias,
			content,
			datetime,
			active
		) VALUES (
			?,
			?,
			?,
			?,
			?,
			?,
			?,
			?
		);`,
		3,
		1,
		"page",
		"About",
		"/about/",
		"<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Feugiat in ante metus dictum at tempor commodo ullamcorper a. Et malesuada fames ac turpis egestas sed tempus urna et. Euismod elementum nisi quis eleifend. Nisi porta lorem mollis aliquam ut porttitor. Ac turpis egestas maecenas pharetra convallis posuere. Nunc non blandit massa enim nec dui. Commodo elit at imperdiet dui accumsan sit amet nulla. Viverra accumsan in nisl nisi scelerisque. Dui nunc mattis enim ut tellus. Molestie ac feugiat sed lectus vestibulum mattis ullamcorper. Faucibus ornare suspendisse sed nisi lacus. Nulla facilisi morbi tempus iaculis. Ut eu sem integer vitae justo eget magna fermentum iaculis. Ullamcorper sit amet risus nullam eget felis eget nunc. Volutpat sed cras ornare arcu dui vivamus. Eget magna fermentum iaculis eu non diam.</p><p>Arcu ac tortor dignissim convallis aenean et tortor. Vitae auctor eu augue ut lectus arcu. Ac turpis egestas integer eget aliquet nibh praesent. Interdum velit euismod in pellentesque massa placerat duis. Vestibulum rhoncus est pellentesque elit ullamcorper dignissim cras tincidunt. Nisl rhoncus mattis rhoncus urna neque viverra justo. Odio ut enim blandit volutpat. Ac auctor augue mauris augue neque gravida. Ut lectus arcu bibendum at varius vel. Porttitor leo a diam sollicitudin tempor id eu nisl nunc. Dolor sit amet consectetur adipiscing elit duis tristique. Semper quis lectus nulla at volutpat diam ut. Sapien eget mi proin sed.</p><p>Ante metus dictum at tempor commodo ullamcorper a. Facilisis mauris sit amet massa vitae. Enim neque volutpat ac tincidunt vitae. Tempus quam pellentesque nec nam aliquam sem. Mollis aliquam ut porttitor leo a diam sollicitudin. Nunc pulvinar sapien et ligula ullamcorper. Dignissim suspendisse in est ante in nibh mauris. Eget egestas purus viverra accumsan in. Vitae tempus quam pellentesque nec nam aliquam sem et. Sodales ut etiam sit amet nisl. Aliquet risus feugiat in ante. Rhoncus urna neque viverra justo nec ultrices dui sapien. Sit amet aliquam id diam maecenas ultricies. Sed odio morbi quis commodo odio aenean sed adipiscing diam.</p>",
		utils.UnixTimestampToMySqlDateTime(utils.GetCurrentUnixTimestamp()),
		1,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("INSERT INTO pages 3: %s", err.Error())
	}

	/*
		if _, err = tx.Exec(
			ctx,
			`INSERT INTO settings (name, value) VALUES ('database_version', '000000024');`,
		); err != nil {
			tx.Rollback()
			return err
		}
	*/

	if _, err = tx.Exec(
		ctx,
		`INSERT INTO shop_cat_product_rel (product_id, category_id)
			VALUES
		(1, 3),
		(2, 3),
		(3, 3);`,
	); err != nil {
		tx.Rollback()
		return err
	}
	if _, err = tx.Exec(
		ctx,
		`INSERT INTO shop_cats (id, user, name, alias, lft, rgt)
			VALUES
		(1, 1, 'ROOT', 'ROOT', 1, 6),
		(2, 1, 'Electronics', 'electronics', 2, 5),
		(3, 1, 'Mobile phones', 'mobile-phones', 3, 4);`,
	); err != nil {
		tx.Rollback()
		return err
	}
	if _, err = tx.Exec(
		ctx,
		`INSERT INTO shop_currencies (id, name, coefficient, code, symbol)
			VALUES
		(1, 'US Dollar', 1.0000, 'USD', '$'),
		(2, 'UA Grivna', 25.0000, 'UAH', '₴');`,
	); err != nil {
		tx.Rollback()
		return err
	}
	if _, err = tx.Exec(
		ctx,
		`INSERT INTO shop_filter_product_values (product_id, filter_value_id)
			VALUES
		(1, 3),
		(1, 7),
		(1, 10),
		(1, 11),
		(1, 12),
		(2, 3),
		(2, 8),
		(2, 10),
		(2, 11),
		(2, 12),
		(3, 3),
		(3, 9),
		(3, 10),
		(3, 11),
		(3, 12);`,
	); err != nil {
		tx.Rollback()
		return err
	}
	if _, err = tx.Exec(
		ctx,
		`INSERT INTO shop_filters (id, name, filter)
			VALUES
		(1, 'Mobile phones manufacturer', 'Manufacturer'),
		(2, 'Mobile phones memory', 'Memory'),
		(3, 'Mobile phones communication standard', 'Communication standard');`,
	); err != nil {
		tx.Rollback()
		return err
	}
	if _, err = tx.Exec(
		ctx,
		`INSERT INTO shop_filters_values (id, filter_id, name)
			VALUES
		(1, 1, 'Apple'),
		(2, 1, 'Asus'),
		(3, 1, 'Samsung'),
		(4, 2, '16 Gb'),
		(5, 2, '32 Gb'),
		(6, 2, '64 Gb'),
		(7, 2, '128 Gb'),
		(8, 2, '256 Gb'),
		(9, 2, '512 Gb'),
		(10, 3, '4G'),
		(11, 3, '2G'),
		(12, 3, '3G');`,
	); err != nil {
		tx.Rollback()
		return err
	}
	if _, err = tx.Exec(
		ctx,
		`INSERT INTO shop_products (
			id,
			user,
			currency,
			price,
			price_old,
			gname,
			name,
			alias,
			vendor,
			quantity,
			category,
			briefly,
			content,
			datetime,
			active
		) VALUES (
			?,
			?,
			?,
			?,
			?,
			?,
			?,
			?,
			?,
			?,
			?,
			?,
			?,
			?,
			?
		);`,
		1,
		1,
		1,
		999.00,
		1100.00,
		"Samsung Galaxy S10",
		"Samsung Galaxy S10 (128 Gb)",
		"samsung-galaxy-s10-128-gb",
		"Samsung",
		"1",
		"3",
		"<p>Arcu ac tortor dignissim convallis aenean et tortor. Vitae auctor eu augue ut lectus arcu. Ac turpis egestas integer eget aliquet nibh praesent.</p>",
		"<p>Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Feugiat in ante metus dictum at tempor commodo ullamcorper a. Et malesuada fames ac turpis egestas sed tempus urna et. Euismod elementum nisi quis eleifend. Nisi porta lorem mollis aliquam ut porttitor. Ac turpis egestas maecenas pharetra convallis posuere. Nunc non blandit massa enim nec dui. Commodo elit at imperdiet dui accumsan sit amet nulla. Viverra accumsan in nisl nisi scelerisque. Dui nunc mattis enim ut tellus. Molestie ac feugiat sed lectus vestibulum mattis ullamcorper. Faucibus ornare suspendisse sed nisi lacus. Nulla facilisi morbi tempus iaculis. Ut eu sem integer vitae justo eget magna fermentum iaculis. Ullamcorper sit amet risus nullam eget felis eget nunc. Volutpat sed cras ornare arcu dui vivamus. Eget magna fermentum iaculis eu non diam.</p><p>Arcu ac tortor dignissim convallis aenean et tortor. Vitae auctor eu augue ut lectus arcu. Ac turpis egestas integer eget aliquet nibh praesent. Interdum velit euismod in pellentesque massa placerat duis. Vestibulum rhoncus est pellentesque elit ullamcorper dignissim cras tincidunt. Nisl rhoncus mattis rhoncus urna neque viverra justo. Odio ut enim blandit volutpat. Ac auctor augue mauris augue neque gravida. Ut lectus arcu bibendum at varius vel. Porttitor leo a diam sollicitudin tempor id eu nisl nunc. Dolor sit amet consectetur adipiscing elit duis tristique. Semper quis lectus nulla at volutpat diam ut. Sapien eget mi proin sed.</p>",
		utils.UnixTimestampToMySqlDateTime(utils.GetCurrentUnixTimestamp()),
		1,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("INSERT INTO shop_products 1: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`INSERT INTO users (
			id, first_name, last_name, email, password, admin, active
		)  VALUES (
			1, 'First Name', 'Last Name', 'example@example.com', '23463b99b62a72f26ed677cc556c44e8', 1, 1
		), (
			2,'admin','Супер пользователь','admin@test.com','d41d8cd98f00b204e9800998ecf8427e', 1, 1
		);
		`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("INSERT INTO users: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`INSERT INTO dashboards 
		VALUES (
			7,4,'dashboard','Диспетчерская схема','/dashboard/Схемы/Диспетчерская схема.xsde_','<p>Тест</p>','','','','2020-09-09 11:25:10',1
		),(
			8,4,'dashboard','События','/dashboard/События/admin.evn_','','Зона ответственности администратора','','','2020-09-23 07:19:46',1
		),(
			9,4,'dashboard','Объектная модель SCADA - системы','/dashboard/WEB-представления/scada','','Объектная модель SCADA - системы','Объектная модель SCADA - системы','Объектная модель SCADA - системы','2020-10-26 12:48:52',1
		),(
			10,4,'dashboard','Карта','/dashboard/Объекты на карте/map','','','','','2021-02-25 10:27:12',1
		);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("INSERT INTO dashboards: %s", err.Error())
	}

	// Indexes
	if _, err = tx.Exec(
		ctx,
		`CREATE UNIQUE INDEX post_category ON  blog_cat_post_rel(post_id,category_id);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE UNIQUE INDEX post_category: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_blog_cat_post_rel_post_id ON  blog_cat_post_rel(post_id);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_blog_cat_post_rel_post_id: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_blog_cat_post_rel_category_id ON  blog_cat_post_rel(category_id);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_blog_cat_post_rel_category_id: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE UNIQUE INDEX alias_blog_cats ON  blog_cats(alias);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE UNIQUE INDEX alias_blog_cats: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`
		CREATE INDEX lft_blog_cats ON  blog_cats(lft);
		CREATE INDEX rgt_blog_cats ON  blog_cats(rgt);
		`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX lft_blog_cats, rgt_blog_cats: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_blog_cats_user ON  blog_cats(user);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_blog_cats_user: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE UNIQUE INDEX alias_blog_posts ON  blog_posts(alias);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE UNIQUE INDEX alias_blog_posts: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_blog_posts_user ON  blog_posts(user);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_blog_posts_user: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_blog_posts_category ON  blog_posts(category);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_blog_posts_category: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX status ON  notify_mail(status);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX status: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE UNIQUE INDEX alias_pages  ON  pages(alias);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE UNIQUE INDEX alias_pages: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX alias_active ON  pages(alias,active);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX alias_active: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_pages_user ON  pages(user);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_pages_user: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE UNIQUE INDEX name ON  settings(name);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE UNIQUE INDEX name: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE UNIQUE INDEX product_category ON  shop_cat_product_rel(product_id, category_id);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE UNIQUE INDEX product_category: %s", err.Error())
	}
	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_shop_cat_product_rel_product_id ON  shop_cat_product_rel(product_id);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_shop_cat_product_rel_product_id: %s", err.Error())
	}
	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_shop_cat_product_rel_category_id ON  shop_cat_product_rel(category_id);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_shop_cat_product_rel_category_id: %s", err.Error())
	}
	if _, err = tx.Exec(
		ctx,
		`CREATE UNIQUE INDEX alias_shop_cats ON  shop_cats(alias);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE UNIQUE INDEX alias_shop_cats: %s", err.Error())
	}
	if _, err = tx.Exec(
		ctx,
		`
		CREATE INDEX lft_shop_cats ON  shop_cats(lft);
		CREATE INDEX rgt_shop_cats ON  shop_cats(rgt);
		`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE UNIQUE INDEX lft_shop_cats, rgt_shop_cats: %s", err.Error())
	}
	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_shop_cats_user ON  shop_cats(user);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_shop_cats_user: %s", err.Error())
	}
	if _, err = tx.Exec(
		ctx,
		`CREATE UNIQUE INDEX product_filter_value ON  shop_filter_product_values(product_id,filter_value_id);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE UNIQUE INDEX product_filter_value: %s", err.Error())
	}
	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_shop_filter_product_values_product_id ON  shop_filter_product_values(product_id);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_shop_filter_product_values_product_id: %s", err.Error())
	}
	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_shop_filter_product_values_filter_value_id ON  shop_filter_product_values(filter_value_id);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_shop_filter_product_values_filter_value_id: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX name_shop_filters ON  shop_filters(name);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX name_shop_filters: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_shop_filters_values_filter_id ON  shop_filters_values(filter_id);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_shop_filters_values_filter_id: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX name_shop_filters_values ON  shop_filters_values(name);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX name_shop_filters_values: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_shop_orders_currency_id ON  shop_orders(currency_id);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_shop_orders_currency_id: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE UNIQUE INDEX order_product ON  shop_order_products(order_id,product_id);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE UNIQUE INDEX order_product: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_shop_order_products_order_id ON  shop_order_products(order_id);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_shop_order_products_order_id: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_shop_order_products_product_id ON  shop_order_products(product_id);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_shop_order_products_product_id: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE UNIQUE INDEX product_filename ON  shop_product_images(product_id,filename);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE UNIQUE INDEX product_filename: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_shop_product_images_product_id ON  shop_product_images(product_id);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_shop_product_images_product_id: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX alias ON  shop_products(alias);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX alias: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_shop_products_user ON  shop_products(user);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_shop_products_user: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_shop_products_currency ON  shop_products(currency);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_shop_products_currency: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_shop_products_category ON  shop_products(category);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_shop_products_category: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX FK_shop_products_parent_id ON  shop_products(parent_id);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX FK_shop_products_parent_id: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE INDEX name_shop_products ON  shop_products(name);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE INDEX name_shop_products: %s", err.Error())
	}

	if _, err = tx.Exec(
		ctx,
		`CREATE UNIQUE INDEX email ON  users(email);`,
	); err != nil {
		tx.Rollback()
		return fmt.Errorf("CREATE UNIQUE INDEX email: %s", err.Error())
	}

	// References
	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE blog_cat_post_rel ADD CONSTRAINT FK_blog_cat_post_rel_post_id
	// 	FOREIGN KEY (post_id) REFERENCES blog_posts (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE blog_cat_post_rel ADD CONSTRAINT FK_blog_cat_post_rel_category_id
	// 	FOREIGN KEY (category_id) REFERENCES blog_cats (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE blog_cats ADD CONSTRAINT FK_blog_cats_user
	// 	FOREIGN KEY (user) REFERENCES users (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE blog_posts ADD CONSTRAINT FK_blog_posts_user
	// 	FOREIGN KEY (user) REFERENCES users (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE blog_posts ADD CONSTRAINT FK_blog_posts_category
	// 	FOREIGN KEY (category) REFERENCES blog_cats (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE pages ADD CONSTRAINT FK_pages_user
	// 	FOREIGN KEY (user) REFERENCES users (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE shop_cat_product_rel ADD CONSTRAINT FK_shop_cat_product_rel_product_id
	// 	FOREIGN KEY (product_id) REFERENCES shop_products (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE shop_cat_product_rel ADD CONSTRAINT FK_shop_cat_product_rel_category_id
	// 	FOREIGN KEY (category_id) REFERENCES shop_cats (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE shop_cats ADD CONSTRAINT FK_shop_cats_user
	// 	FOREIGN KEY (user) REFERENCES users (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE shop_filter_product_values ADD CONSTRAINT FK_shop_filter_product_values_product_id
	// 	FOREIGN KEY (product_id) REFERENCES shop_products (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE shop_filter_product_values ADD CONSTRAINT FK_shop_filter_product_values_filter_value_id
	// 	FOREIGN KEY (filter_value_id) REFERENCES shop_filters_values (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE shop_filters_values ADD CONSTRAINT FK_shop_filters_values_filter_id
	// 	FOREIGN KEY (filter_id) REFERENCES shop_filters (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE shop_orders ADD CONSTRAINT FK_shop_orders_currency_id
	// 	FOREIGN KEY (currency_id) REFERENCES shop_currencies (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE shop_order_products ADD CONSTRAINT FK_shop_order_products_order_id
	// 	FOREIGN KEY (order_id) REFERENCES shop_orders (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE shop_order_products ADD CONSTRAINT FK_shop_order_products_product_id
	// 	FOREIGN KEY (product_id) REFERENCES shop_products (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE shop_product_images ADD CONSTRAINT FK_shop_product_images_product_id
	// 	FOREIGN KEY (product_id) REFERENCES shop_products (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE shop_products ADD CONSTRAINT FK_shop_products_user
	// 	FOREIGN KEY (user) REFERENCES users (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE shop_products ADD CONSTRAINT FK_shop_products_currency
	// 	FOREIGN KEY (currency) REFERENCES shop_currencies (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE shop_products ADD CONSTRAINT FK_shop_products_category
	// 	FOREIGN KEY (category) REFERENCES shop_cats (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// if _, err = tx.Exec(
	// 	ctx,
	// 	`ALTER TABLE shop_products ADD CONSTRAINT FK_shop_products_parent_id
	// 	FOREIGN KEY (parent_id) REFERENCES shop_products (id) ON DELETE RESTRICT;
	// `); err != nil {
	// 	tx.Rollback()
	// 	return err
	// }

	// Commit all changes
	err = tx.Commit()
	if err != nil {
		return err
	}

	return nil

}
