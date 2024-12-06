#include "core/View.h"

#include "core/ViewImpl.h"

IView::Ptr createView(IModel::Ptr m_ptr) {
    return std::make_unique<ViewImpl>(std::move(m_ptr));
}