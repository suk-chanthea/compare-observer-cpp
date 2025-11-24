#ifndef STYLES_H
#define STYLES_H

#include <QString>

/**
 * @brief UI styles for the application
 */
namespace Styles {
    /**
     * @brief Gets the main stylesheet for the application
     * @return CSS stylesheet string
     */
    QString getMainStylesheet();

    /**
     * @brief Gets stylesheet for dialogs
     * @return CSS stylesheet string
     */
    QString getDialogStylesheet();

    /**
     * @brief Gets stylesheet for tables
     * @return CSS stylesheet string
     */
    QString getTableStylesheet();
}

#endif // STYLES_H
