/*
 * @file: adaboost_main.cpp
 * @author: Udit Saxena
 *
 * Implementation of the AdaBoost main program.
 *
 * @code
 * @article{Schapire:1999:IBA:337859.337870,
 *   author = {Schapire, Robert E. and Singer, Yoram},
 *   title = {Improved Boosting Algorithms Using Confidence-rated Predictions},
 *   journal = {Machine Learning},
 *   issue_date = {Dec. 1999},
 *   volume = {37},
 *   number = {3},
 *   month = dec,
 *   year = {1999},
 *   issn = {0885-6125},
 *   pages = {297--336},
 *   numpages = {40},
 *   url = {http://dx.doi.org/10.1023/A:1007614523901},
 *   doi = {10.1023/A:1007614523901},
 *   acmid = {337870},
 *   publisher = {Kluwer Academic Publishers},
 *   address = {Hingham, MA, USA},
 *   keywords = {boosting algorithms, decision trees, multiclass classification,
 *   output coding}
 * @endcode
 *
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */

#include <mlpack/prereqs.hpp>
#include <mlpack/core/util/param.hpp>
#include <mlpack/core/data/normalize_labels.hpp>
#include "adaboost.hpp"
#include <mlpack/core/data/load.hpp>
#include <mlpack/core/data/save.hpp>

using namespace mlpack;
using namespace std;
using namespace arma;
using namespace mlpack::adaboost;
using namespace mlpack::decision_stump;
using namespace mlpack::perceptron;

PROGRAM_INFO("AdaBoost", "This program implements the AdaBoost (or Adaptive "
    "Boosting) algorithm. The variant of AdaBoost implemented here is "
    "AdaBoost.MH. It uses a weak learner, either decision stumps or "
    "perceptrons, and over many iterations, creates a strong learner that is a "
    "weighted ensemble of weak learners. It runs these iterations until a "
    "tolerance value is crossed for change in the value of the weighted "
    "training error."
    "\n\n"
    "For more information about the algorithm, see the paper \"Improved "
    "Boosting Algorithms Using Confidence-Rated Predictions\", by R.E. Schapire"
    " and Y. Singer."
    "\n\n"
    "This program allows training of an AdaBoost model, and then application of"
    " that model to a test dataset.  To train a model, a dataset must be passed"
    " with the --training_file (-t) option.  Labels can be given with the "
    "--labels_file (-l) option; if no labels file is specified, the labels will"
    " be assumed to be the last column of the input dataset.  Alternately, an "
    "AdaBoost model may be loaded with the --input_model_file (-m) option."
    "\n\n"
    "Once a model is trained or loaded, it may be used to provide class "
    "predictions for a given test dataset.  A test dataset may be specified "
    "with the --test_file (-T) parameter.  The predicted classes for each point"
    " in the test dataset will be saved into the file specified by the "
    "--output_file (-o) parameter.  The AdaBoost model itself may be saved to "
    "a file specified by the --output_model_file (-M) parameter.");

// Input for training.
PARAM_STRING_IN("training_file", "A file containing the training set.", "t",
    "");
PARAM_STRING_IN("labels_file", "A file containing labels for the training set.",
    "l", "");

// Loading/saving of a model.
PARAM_STRING_IN("input_model_file", "File containing input AdaBoost model.",
    "m", "");
PARAM_STRING_OUT("output_model_file", "File to save trained AdaBoost model to.",
    "M");

// Classification options.
PARAM_STRING_IN("test_file", "A file containing the test set.", "T", "");
PARAM_STRING_OUT("output_file", "The file in which the predicted labels for the"
    " test set will be written.", "o");

// Training options.
PARAM_INT_IN("iterations", "The maximum number of boosting iterations to be run"
    " (0 will run until convergence.)", "i", 1000);
PARAM_DOUBLE_IN("tolerance", "The tolerance for change in values of the "
    "weighted error during training.", "e", 1e-10);
PARAM_STRING_IN("weak_learner", "The type of weak learner to use: "
    "'decision_stump', or 'perceptron'.", "w", "decision_stump");

/**
 * The model to save to disk.
 */
class AdaBoostModel
{
 public:
  enum WeakLearnerTypes
  {
    DECISION_STUMP,
    PERCEPTRON
  };

 private:
  //! The mappings for the labels.
  Col<size_t> mappings;
  //! The type of weak learner.
  size_t weakLearnerType;
  //! Non-NULL if using decision stumps.
  AdaBoost<DecisionStump<>>* dsBoost;
  //! Non-NULL if using perceptrons.
  AdaBoost<Perceptron<>>* pBoost;
  //! Number of dimensions in training data.
  size_t dimensionality;

 public:
  //! Create an empty AdaBoost model.
  AdaBoostModel() : dsBoost(NULL), pBoost(NULL), dimensionality(0) { }

  //! Create the AdaBoost model with the given mappings and type.
  AdaBoostModel(const Col<size_t>& mappings, const size_t weakLearnerType) :
      mappings(mappings),
      weakLearnerType(weakLearnerType),
      dsBoost(NULL),
      pBoost(NULL),
      dimensionality(0)
  {
    // Nothing to do.
  }

  ~AdaBoostModel()
  {
    if (dsBoost)
      delete dsBoost;
    if (pBoost)
      delete pBoost;
  }

  //! Get the mappings.
  const Col<size_t>& Mappings() const { return mappings; }
  //! Modify the mappings.
  Col<size_t>& Mappings() { return mappings; }

  //! Get the weak learner type.
  size_t WeakLearnerType() const { return weakLearnerType; }
  //! Modify the weak learner type.
  size_t& WeakLearnerType() { return weakLearnerType; }

  //! Get the dimensionality of the model.
  size_t Dimensionality() const { return dimensionality; }
  //! Modify the dimensionality of the model.
  size_t& Dimensionality() { return dimensionality; }

  //! Train the model.
  void Train(const mat& data,
             const Row<size_t>& labels,
             const size_t iterations,
             const double tolerance)
  {
    dimensionality = data.n_rows;
    if (weakLearnerType == WeakLearnerTypes::DECISION_STUMP)
    {
      if (dsBoost)
        delete dsBoost;

      DecisionStump<> ds(data, labels, max(labels) + 1);
      dsBoost = new AdaBoost<DecisionStump<>>(data, labels, ds, iterations,
          tolerance);
    }
    else if (weakLearnerType == WeakLearnerTypes::PERCEPTRON)
    {
      Perceptron<> p(data, labels, max(labels) + 1);
      pBoost = new AdaBoost<Perceptron<>>(data, labels, p, iterations,
          tolerance);
    }
  }

  //! Classify test points.
  void Classify(const mat& testData, Row<size_t>& predictions)
  {
    if (weakLearnerType == WeakLearnerTypes::DECISION_STUMP)
      dsBoost->Classify(testData, predictions);
    else if (weakLearnerType == WeakLearnerTypes::PERCEPTRON)
      pBoost->Classify(testData, predictions);
  }

  //! Serialize the model.
  template<typename Archive>
  void Serialize(Archive& ar, const unsigned int /* version */)
  {
    if (Archive::is_loading::value)
    {
      if (dsBoost)
        delete dsBoost;
      if (pBoost)
        delete pBoost;

      dsBoost = NULL;
      pBoost = NULL;
    }

    ar & data::CreateNVP(mappings, "mappings");
    ar & data::CreateNVP(weakLearnerType, "weakLearnerType");
    if (weakLearnerType == WeakLearnerTypes::DECISION_STUMP)
      ar & data::CreateNVP(dsBoost, "adaboost_ds");
    else if (weakLearnerType == WeakLearnerTypes::PERCEPTRON)
      ar & data::CreateNVP(pBoost, "adaboost_p");
    ar & data::CreateNVP(dimensionality, "dimensionality");
  }
};

int main(int argc, char *argv[])
{
  CLI::ParseCommandLine(argc, argv);

  // Check input parameters and issue warnings/errors as necessary.

  // The user cannot specify both a training file and an input model file.
  if (CLI::HasParam("training_file") && CLI::HasParam("input_model_file"))
  {
    Log::Fatal << "Only one of --training_file or --input_model_file may be "
        << "specified!" << endl;
  }

  // The user must specify either a training file or an input model file.
  if (!CLI::HasParam("training_file") && !CLI::HasParam("input_model_file"))
  {
    Log::Fatal << "Either --training_file or --input_model_file must be "
        << "specified!" << endl;
  }

  // The weak learner must make sense.
  if (CLI::GetParam<string>("weak_learner") != "decision_stump" &&
      CLI::GetParam<string>("weak_learner") != "perceptron")
  {
    Log::Fatal << "Unknown weak learner type '"
        << CLI::GetParam<string>("weak_learner")
        << "'; must be 'decision_stump' or 'perceptron'." << endl;
  }

  // --labels_file can't be specified without --training_file.
  if (CLI::HasParam("labels_file") && !CLI::HasParam("training_file"))
    Log::Warn << "--labels_file ignored, because --training_file was not "
        << "passed." << endl;

  // Sanity check on iterations.
  int iterInt = CLI::GetParam<int>("iterations");
  if (iterInt < 0)
  {
    Log::Fatal << "Invalid number of iterations (" << iterInt << ") specified! "
        << "Must be greater than 0." << endl;
  }

  // If a weak learner is specified with a model, it will be ignored.
  if (CLI::HasParam("input_model_file") && CLI::HasParam("weak_learner"))
  {
    Log::Warn << "--weak_learner ignored because --input_model_file is "
        << "specified." << endl;
  }

  // Training parameters are ignored if no training file is given.
  if (CLI::HasParam("tolerance") && !CLI::HasParam("training_file"))
  {
    Log::Warn << "--tolerance ignored, because --training_file was not "
        << "passed." << endl;
  }
  if (CLI::HasParam("iterations") && !CLI::HasParam("training_file"))
  {
    Log::Warn << "--iterations ignored, because --training_file was not "
        << "passed." << endl;
  }

  if (!CLI::HasParam("output_model_file") && !CLI::HasParam("output_file"))
  {
    Log::Warn << "Neither --output_model_file nor --output_file are specified; "
        << "no results will be saved." << endl;
  }

  if (CLI::HasParam("output_file") && !CLI::HasParam("test_file"))
  {
    Log::Warn << "--output_file ignored because --test_file is not specified."
        << endl;
  }

  AdaBoostModel m;
  if (CLI::HasParam("training_file"))
  {
    const string trainingDataFilename = CLI::GetParam<string>("training_file");
    mat trainingData;
    data::Load(trainingDataFilename, trainingData, true);

    const string labelsFilename = CLI::GetParam<string>("labels_file");
    // Load labels.
    Mat<size_t> labelsIn;

    if (CLI::HasParam("labels_file"))
    {
      const string labelsFilename = CLI::GetParam<string>("labels_file");
      // Load labels.
      data::Load(labelsFilename, labelsIn, true);

      // Do the labels need to be transposed?
      if (labelsIn.n_cols == 1)
        labelsIn = labelsIn.t();
    }
    else
    {
      // Extract the labels as the last dimension of the training data.
      Log::Info << "Using the last dimension of training set as labels."
          << endl;
      labelsIn = conv_to<Mat<size_t>>::from(
          trainingData.row(trainingData.n_rows - 1)).t();
      trainingData.shed_row(trainingData.n_rows - 1);
    }

    // Helpers for normalizing the labels.
    Row<size_t> labels;

    // Normalize the labels.
    data::NormalizeLabels(labelsIn.row(0), labels, m.Mappings());

    // Get other training parameters.
    const double tolerance = CLI::GetParam<double>("tolerance");
    const size_t iterations = (size_t) CLI::GetParam<int>("iterations");
    const string weakLearner = CLI::GetParam<string>("weak_learner");
    if (weakLearner == "decision_stump")
      m.WeakLearnerType() = AdaBoostModel::WeakLearnerTypes::DECISION_STUMP;
    else if (weakLearner == "perceptron")
      m.WeakLearnerType() = AdaBoostModel::WeakLearnerTypes::PERCEPTRON;

    Timer::Start("adaboost_training");
    m.Train(trainingData, labels, iterations, tolerance);
    Timer::Stop("adaboost_training");
  }
  else
  {
    // We have a specified input model file.
    const string inputModelFile = CLI::GetParam<string>("input_model_file");
    data::Load(inputModelFile, "adaboost_model", m, true); // Fatal on failure.
  }

  // Perform classification, if desired.
  if (CLI::HasParam("test_file"))
  {
    const string testingDataFilename = CLI::GetParam<string>("test_file");
    mat testingData;
    data::Load(testingDataFilename, testingData, true);

    if (testingData.n_rows != m.Dimensionality())
      Log::Fatal << "Test data dimensionality (" << testingData.n_rows << ") "
          << "must be the same as the model dimensionality ("
          << m.Dimensionality() << ")!" << endl;

    Row<size_t> predictedLabels(testingData.n_cols);
    Timer::Start("adaboost_classification");
    m.Classify(testingData, predictedLabels);
    Timer::Stop("adaboost_classification");

    Row<size_t> results;
    data::RevertLabels(predictedLabels, m.Mappings(), results);

    if (CLI::HasParam("output_file"))
      data::Save(CLI::GetParam<string>("output_file"), results, true);
  }

  // Should we save the model, too?
  if (CLI::HasParam("output_model_file"))
    data::Save(CLI::GetParam<string>("output_model_file"), "adaboost_model", m);
}
