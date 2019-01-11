/*This is the draft main part of the code to train a model to classify patches. 
  Here, the main idea is to classify each patch. The model is trained in Dlib in 
  C++ and the overall accuracy for 10 images is about 55 percent. The results are 
  not acceptable yet however by data augmentation and better training images (or
  parameter optimization) it would definitely achieve good results. */
  template <
	int N,
	template <typename> class BN,
	int stride,
	typename SUBNET
>
using block = BN<con<N, 3, 3, 1, 1, relu<BN<con<N, 3, 3, stride, stride, SUBNET>>>>>;

template <
	template <int, template<typename>class, int, typename> class block,
	int N,
	template<typename>class BN,
	typename SUBNET
>
using residual = add_prev1<block<N, BN, 1, tag1<SUBNET>>>;

template <
	template <int, template<typename>class, int, typename> class block,
	int N,
	template<typename>class BN,
	typename SUBNET
>
using residual_down = add_prev2<avg_pool<2, 2, 2, 2, skip1<tag2<block<N, BN, 2, tag1<SUBNET>>>>>>;

static std::vector<dlib::matrix<dlib::rgb_pixel>> images;
template <typename SUBNET> using res = relu<residual<block, 8, bn_con, SUBNET>>;
template <typename SUBNET> using ares = relu<residual<block, 8, affine, SUBNET>>;
template <typename SUBNET> using res_down = relu<residual_down<block, 8, bn_con, SUBNET>>;
template <typename SUBNET> using ares_down = relu<residual_down<block, 8, affine, SUBNET>>;

const unsigned long number_of_classes = 2;
using net_type = loss_multiclass_log<fc<number_of_classes,
	avg_pool_everything<
	res<res<res<res_down<
	res<res<res<res<res<res<res<res<res< // repeat this layer 9 times
	res_down<
	res<
	input<matrix<rgb_pixel>>
	>>>>>>>>>>>>>>>>>>;

template <typename SUBNET>
using pres = prelu<add_prev1<bn_con<con<8, 3, 3, 1, 1, prelu<bn_con<con<8, 3, 3, 1, 1, tag1<SUBNET>>>>>>>>;
  
  int row = 101;
	int col = 101;
	int train_images_num = 1000;
	int train_images_mitosis = 500;
	stringstream str;
	std::vector<unsigned long> train_labels;
	std::vector<matrix<rgb_pixel>> train_images;
	std::vector<matrix<rgb_pixel>> train_images_resize;

	train_images.clear();
	train_images.resize(train_images_num);
	cout << "before resize train images" << endl;
	for (int i = 0; i < train_images_num; i++) { train_images[i].set_size(row, col); }
	cout << "after resize train images" << endl;
	train_images_resize.clear();
	train_images_resize.resize(train_images_num);
	train_labels.clear();
	train_labels.resize(train_images_num);
	cout << "before loading train images" << endl;
	for (size_t i = 0; i < train_images_num; ++i)
	{
		str << trainInputDirDL << i << ".jpg";
		load_image(train_images_resize[i], str.str());
		resize_image(train_images_resize[i], train_images[i], interpolate_bilinear());
		if (i < train_images_mitosis)
		{
			cout << "mitosis" << endl;
			train_labels[i] = 0;
		}
		else
		{
			cout << "nonmitosis" << endl;
			train_labels[i] = 1;
		}
		str.str("");
		cout << "i: " << i << endl;
	}
	cout << "before model" << endl;




	net_type net;
	dnn_trainer<net_type, adam> trainer(net, adam(0.0005, 0.9, 0.999));
	trainer.set_iterations_without_progress_threshold(2000);
	trainer.set_learning_rate_shrink_factor(0.1);
	// The learning rate will start at 1e-3.
	trainer.set_learning_rate(1e-3);
	trainer.be_verbose();
	cout << trainer << endl;

	dlib::rand rnd; // gives error 
	for (auto&& img : train_images)
		disturb_colors(img, rnd);

	cout << "before train" << endl;
	trainer.set_synchronization_file(trainDataDir + "mitosis_network_sync", std::chrono::seconds(60));
	trainer.train(train_images, train_labels);
	cout << "after train" << endl;
	net.clean();
	serialize(trainDataDir + "mitosis_network.dat") << net;

	std::vector<unsigned long> predicted_labels = net(train_images);
	int num_right = 0;
	int num_wrong = 0;
	// And then let's see if it classified them correctly.
	for (size_t i = 0; i < train_images_resize.size(); ++i) // ++i 
	{
		if (predicted_labels[i] == train_labels[i])
			++num_right;
		else
			++num_wrong;

	}
	cout << "training num_right: " << num_right << endl;
	cout << "training num_wrong: " << num_wrong << endl;
	cout << "training accuracy:  " << num_right / (double)(num_right + num_wrong) << endl;
	
	std::vector<matrix<rgb_pixel>> test_images;
	std::vector<unsigned long> test_labels;
	int test_images_num = 13;
	test_images.clear();
	test_images.resize(test_images_num);
	test_labels.clear();
	test_labels.resize(test_images_num);
	std::vector<matrix<rgb_pixel>> test_images_resize;
	test_images_resize.clear();
	test_images_resize.resize(test_images_num);
	num_right = 0;
	num_wrong = 0;
	int test_images_mitosis = 4;
	stringstream str2;
	for (int i = 0; i < test_images_num; ++i)
	{
		str2 << outputDirDL << i << ".jpg";
		load_image(test_images_resize[i], str2.str());
		resize_image(test_images_resize[i], test_images[i], interpolate_bilinear());
		if (i < test_images_mitosis)
		{
			//cout << "mitosis" << endl;
			test_labels[i] = 0;
		}
		else
		{
			//cout << "nonmitosis" << endl;
			test_labels[i] = 1;
		}

		str2.str("");
	}
	cout << "before predicted labels: " << endl;
	std::vector<unsigned long> predicted_labels_test = net(test_images);
	cout << "after predicted labels: " << endl;
	for (size_t i = 0; i < test_images.size(); ++i)
	{
		cout << "predicted results: " << predicted_labels_test[i] << endl;
		if (predicted_labels_test[i] == test_labels[i])
			++num_right;
		else
			++num_wrong;

	}
	cout << "testing num_right: " << num_right << endl;
	cout << "testing num_wrong: " << num_wrong << endl;
	cout << "testing accuracy:  " << num_right / (double)(num_right + num_wrong) << endl;