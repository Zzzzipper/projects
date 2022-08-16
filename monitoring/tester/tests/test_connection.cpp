#include <gtest/gtest.h>

#include "test_boost.h"

using namespace std;

void TestObj::runThread1()
{
	boost::this_thread::sleep(boost::posix_time::seconds(1));

	boost::shared_ptr<int> temp = user_.lock();
    boost::shared_ptr<int> temp1 = boost::static_pointer_cast<int>(temp);

	if (temp) {
		printf("temp. \n");
        *temp = 50;
	}
	else {
		printf("temp is null. \n");
	}

	return;
}

TEST(test_thread, TestThreadSync)
{
    TestObj test_obj;
    boost::shared_ptr<int> temp(new int);
    *temp = 100;
    
    test_obj.user_ = temp;
    test_obj.thread1_ = boost::thread(&TestObj::runThread1, &test_obj);
    
    //boost::this_thread::sleep(boost::posix_time::seconds(2));
    temp.reset();
    boost::this_thread::sleep(boost::posix_time::seconds(5));
    
    if (temp)
        printf("main, temp=%d. \n", *temp);

    boost::shared_ptr<int> temp2 = test_obj.user_.lock();
    if (temp2)
        printf("main, temp=%d. \n", *temp2);
    
    test_obj.thread1_.join();
}

int bind_call(int a, char *b)
{
    printf("[%s()] a=%d, b=%s. \n", __FUNCTION__, a, b);
    
    return a;
}

void test_bind()
{
    boost::function<int (int, char *)> call_func_;
    
    // error
    // void *func_ptr = (void *)bind_call;
    // call_func_ = bind(func_ptr, _1, _2);
    
    call_func_ = boost::bind(bind_call, _1, _2);
    
    call_func_(1, "james");
    
    demo_class dc;
    call_back_factory cbf;

    dc.accept(boost::bind(&call_back_factory::call_back_func1, &cbf, _1));
    dc.run();
    
    dc.accept(boost::bind(&call_back_factory::call_back_func2, cbf, _1, 5));
    dc.run();
}

TEST(test_xml, TestPropertyTree)
{
    struct _def_account
    {
        string username;
        string password;
    };

    struct _model_def_account
    {
        struct def_account;
        string model_name;
    };

    struct _model_def_account model_def_account;
    boost::property_tree::ptree head_tree, model_child;
    stringstream ss;

    try {
        boost::property_tree::xml_parser::read_xml("../model_def_account.xml", head_tree);
    }
    catch(boost::exception const&  ex) {
        std::cout << "read_xml error." << std::endl;
        return;
    }
    
    if (head_tree.empty())
        return;

    model_child = head_tree.get_child("model");

    for (boost::property_tree::ptree::iterator itr=model_child.begin(); itr!=model_child.end(); itr++) {
        boost::property_tree::ptree account = itr->second;
        ss.str(""); ss << "username=" << account.get<string>("username") << ", password=" << account.get<string>("password") << ", list_name=" << account.get<string>("list_name");
        std::cout << ss.str() << std::endl;
    }
}

int main(int argc, char **argv)
{
	std::cout << "ready go to test boost!." << std::endl;

	testing::InitGoogleTest(&argc, argv);

	RUN_ALL_TESTS();

	std::cin.get();

	return EXIT_SUCCESS;
}
