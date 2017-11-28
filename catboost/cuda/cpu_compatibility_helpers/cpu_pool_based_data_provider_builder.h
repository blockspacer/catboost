#pragma once

#include "externel_cat_values_holder.h"
#include <catboost/libs/data/pool.h>
#include <catboost/libs/logging/logging.h>
#include <catboost/cuda/data/data_provider.h>
#include <catboost/cuda/data/binarizations_manager.h>
#include <catboost/cuda/data/cat_feature_perfect_hash_helper.h>
#include <catboost/cuda/data/grid_creator.h>
#include <catboost/cuda/cuda_util/cpu_random.h>

namespace NCatboostCuda
{
    class TCpuPoolBasedDataProviderBuilder
    {
    public:

        TCpuPoolBasedDataProviderBuilder(TBinarizedFeaturesManager& featureManager,
                                         const TPool& pool,
                                         bool isTest,
                                         TDataProvider& dst)
                : FeaturesManager(featureManager)
                  , DataProvider(dst)
                  , Pool(pool)
                  , IsTest(isTest)
                  , CatFeaturesPerfectHashHelper(FeaturesManager)
        {
            DataProvider.Targets = pool.Docs.Target;
            DataProvider.Weights = pool.Docs.Weight;

            const ui32 numSamples = pool.Docs.GetDocCount();

            DataProvider.QueryIds.resize(numSamples);
            for (ui32 i = 0; i < DataProvider.QueryIds.size(); ++i)
            {
                DataProvider.QueryIds[i] = i;
            }

            DataProvider.Baseline.resize(pool.Docs.Baseline.size());
            for (ui32 i = 0; i < pool.Docs.Baseline.size(); ++i)
            {
                auto& baseline = DataProvider.Baseline[i];
                auto& baselineSrc = pool.Docs.Baseline[i];
                baseline.resize(baselineSrc.size());
                for (ui32 j = 0; j < baselineSrc.size(); ++j)
                {
                    baseline[j] = baselineSrc[j];
                }
            }
        }

        template<class TContainer>
        TCpuPoolBasedDataProviderBuilder& AddIgnoredFeatures(const TContainer& container)
        {
            for (const auto& f : container)
            {
                IgnoreFeatures.insert(f);
            }
            return *this;
        }

        TCpuPoolBasedDataProviderBuilder& SetClassesWeights(const TVector<float>& weights) {
            ClassesWeights = weights;
            return *this;
        }

        void Finish(ui32 binarizationThreads);

    private:

        void RegisterFeaturesInFeatureManager(const yset<int>& catFeatureIds) const
        {
            const ui32 factorsCount = Pool.Docs.GetFactorsCount();
            for (ui32 featureId = 0; featureId < factorsCount; ++featureId)
            {
                if (!FeaturesManager.IsKnown(featureId))
                {
                    if (catFeatureIds.has(featureId))
                    {
                        FeaturesManager.RegisterDataProviderCatFeature(featureId);
                    } else
                    {
                        FeaturesManager.RegisterDataProviderFloatFeature(featureId);
                    }
                }
            }
        }

    private:
        TBinarizedFeaturesManager& FeaturesManager;
        TDataProvider& DataProvider;
        const TPool& Pool;
        bool IsTest ;
        TCatFeaturesPerfectHashHelper CatFeaturesPerfectHashHelper;
        yset<ui32> IgnoreFeatures;
        TVector<float> ClassesWeights;
    };

}
