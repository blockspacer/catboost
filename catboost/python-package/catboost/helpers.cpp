#include <Python.h>

#include "helpers.h"
#include "catboost/libs/helpers/exception.h"
#include <catboost/libs/helpers/interrupt.h>

extern "C" PyObject* PyCatboostExceptionType;

void ProcessException() {
    try {
        throw;
    } catch (const TCatboostException& exc) {
        PyErr_SetString(PyCatboostExceptionType, exc.what());
    } catch (const TInterruptException& exc) {
        PyErr_SetString(PyExc_KeyboardInterrupt, exc.what());
    } catch (const std::exception& exc) {
        PyErr_SetString(PyCatboostExceptionType, exc.what());
    }
}

void PyCheckInterrupted() {
    TGilGuard guard;
    if (PyErr_CheckSignals() == -1) {
        throw TInterruptException();
    }
}

void SetPythonInterruptHandler() {
    SetInterruptHandler(PyCheckInterrupted);
}

void ResetPythonInterruptHandler() {
    ResetInterruptHandler();
}

TVector<TVector<double>> EvalMetrics(
    const TFullModel& model,
    const TPool& pool,
    const TString& metricDescription,
    int begin,
    int end,
    int evalPeriod,
    int threadCount,
    const TString& tmpDir
) {
    TVector<THolder<IMetric>> metrics;
    NPar::TLocalExecutor executor;
    executor.RunAdditionalThreads(threadCount - 1);

    TMetricsPlotCalcer plotCalcer = CreateMetricCalcer(
        model,
        metricDescription,
        begin,
        end,
        evalPeriod,
        executor,
        tmpDir,
        &metrics
    );
    plotCalcer.ProceedDataSet(pool);

    TVector<TVector<double>> metricsScore = plotCalcer.GetMetricsScore();

    plotCalcer.ClearTempFiles();
    return metricsScore;
}
