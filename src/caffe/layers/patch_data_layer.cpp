#ifdef USE_OPENCV
#include <opencv2/core/core.hpp>
#endif  // USE_OPENCV
#include <stdint.h>

#include <vector>

//#include "caffe/data_transformer.hpp"
#include "caffe/layers/patch_data_layer.hpp"
#include "caffe/util/benchmark.hpp"

namespace caffe {

template <typename Dtype>
PatchDataLayer<Dtype>::PatchDataLayer(const LayerParameter& param)
  : BasePrefetchingDataLayer<Dtype>(param),
    patch_sampler_(param) {
}

template <typename Dtype>
PatchDataLayer<Dtype>::~PatchDataLayer() {
  this->StopInternalThread();
}

template <typename Dtype>
void PatchDataLayer<Dtype>::DataLayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top) {

    LOG(INFO)<<"start dataLayer_SetUp ";
  const int batch_size = this->layer_param_.patch_sampler_param().batch_size();
  // Read a data point, and use it to initialize the top blob.
  //Datum& datum = *(reader_.full().peek());

  // Use data_transformer to infer the expected blob shape from datum.
//  vector<int> top_shape = this->data_transformer_->InferBlobShape(datum);
//  this->transformed_data_.Reshape(top_shape);
  // Reshape top[0] and prefetch_data according to the batch_size.

  // top_data_shape.push_back(batch_size);
  // for(int i=1;i<this->layer_param_patch_sample_param().patch_shape_size();++i){
  //   top_data_shape.push_back(this->layer_param_patch_sample_param().patch_shape(i))
  // }
  LOG(INFO)<<"get shape ";
  vector<int>& top_data_shape=patch_sampler_.patch_data_shape();
  top_data_shape[0]=batch_size;
  for(int i=0;i<top_data_shape.size();i++)
     LOG(INFO)<<"reshape top  = " << top_data_shape[i];
  LOG(INFO)<<"reshape top ";
  top[0]->Reshape(top_data_shape);
  LOG(INFO)<<"reshape prefetch_[i].data_ ";
  for (int i = 0; i < this->PREFETCH_COUNT; ++i) {
    this->prefetch_[i].data_.Reshape(top_data_shape);
  }
  // LOG(INFO) << "output data size: " << top[0]->num() << ","
  //     << top[0]->channels() << "," << top[0]->height() << ","
  //     << top[0]->width();
  // label
  LOG(INFO)<<"reshape prefetch_[i].label_ ";
  if (this->output_labels_) {
    vector<int> top_label_shape=patch_sampler_.patch_label_shape();
    top_label_shape[0]=batch_size;
    top[1]->Reshape(top_label_shape);
    for (int i = 0; i < this->PREFETCH_COUNT; ++i) {
      this->prefetch_[i].label_.Reshape(top_label_shape);
    }
  }
}

// This function is called on prefetch thread
template<typename Dtype>
void PatchDataLayer<Dtype>::load_batch(Batch<Dtype>* batch) {
  CPUTimer batch_timer;
  batch_timer.Start();
  double read_time = 0;
  double trans_time = 0;
  CPUTimer timer;
  CHECK(batch->data_.count());
  //CHECK(this->transformed_data_.count());

  // Reshape according to the first datum of each batch
  // on single input batches allows for inputs of varying dimension.
  const int batch_size = this->layer_param_.patch_sampler_param().batch_size();

  vector<int> top_label_shape=patch_sampler_.patch_label_shape();
  top_label_shape[0]=batch_size;

  vector<int> top_data_shape=patch_sampler_.patch_data_shape();
  top_data_shape[0]=batch_size;

  //Datum& datum = *(reader_.full().peek());
  // Use data_transformer to infer the expected blob shape from datum.
  //vector<int> top_shape = this->data_transformer_->InferBlobShape(datum);
  //this->transformed_data_.Reshape(top_shape);
  // Reshape batch according to the batch_size.
  //top_shape[0] = batch_size;
  batch->data_.Reshape(top_data_shape);

  Dtype* top_data = batch->data_.mutable_cpu_data();
  Dtype* top_label = NULL;  // suppress warnings about uninitialized variables

  if (this->output_labels_) {
    batch->label_.Reshape(top_label_shape);
    top_label = batch->label_.mutable_cpu_data();
  }
  for (int item_id = 0; item_id < batch_size; ++item_id) {
    timer.Start();
    // get a datum
    //Datum& datum = *(reader_.full().pop("Waiting for data"));
    read_time += timer.MicroSeconds();
    timer.Start();
    // Apply data transformations (mirror, scale, crop...)
    vector<int> v_item_id(1);//(1,item_id);
    v_item_id[0]=item_id;
    int d_offset = batch->data_.offset(v_item_id);
    int l_offset = batch->label_.offset(v_item_id);

    // LOG(INFO) <<"batch data offset =  "<<d_offset;
    //   LOG(INFO) <<"batch label offset =  "<<l_offset;
      //  sleep(100);
    //this->transformed_data_.set_cpu_data(top_data + offset);
    //this->data_transformer_->Transform(datum, &(this->transformed_data_));

    int d_length =1;
    vector<int> d_shape =batch->data_.shape();
    //   LOG(INFO) <<"batch data shape "<<d_shape[0];
    // for(int i=1;i<d_shape.size();i++)
    //    { d_length *=d_shape[i];
    //     LOG(INFO) <<"batch data shape "<<d_shape[i];
    //  }


    // Copy label.
    // if (this->output_labels_) {
    //   top_label[item_id] = datum.label();
    // }


    trans_time += timer.MicroSeconds();
    Batch_data<Dtype>* data_label=patch_sampler_.full().pop();

    //CHECK_EQ(d_offset-d_length*item_id,data_label->data_->count());
    const Dtype* source_data  = data_label->data_->cpu_data();
    const Dtype* source_label = data_label->label_->cpu_data();
    caffe_copy(data_label->data_->count(), source_data, top_data+d_offset);
    if(this->output_labels_){
      caffe_copy(data_label->label_->count(), source_label, top_label+l_offset);
    }
    patch_sampler_.free().push(data_label);
    //reader_.free().push(const_cast<Datum*>(&datum));
  }
  timer.Stop();
  batch_timer.Stop();
  DLOG(INFO) << "Prefetch batch: " << batch_timer.MilliSeconds() << " ms.";
  DLOG(INFO) << "     Read time: " << read_time / 1000 << " ms.";
  DLOG(INFO) << "Transform time: " << trans_time / 1000 << " ms.";
}

INSTANTIATE_CLASS(PatchDataLayer);
REGISTER_LAYER_CLASS(PatchData);

}  // namespace caffe
