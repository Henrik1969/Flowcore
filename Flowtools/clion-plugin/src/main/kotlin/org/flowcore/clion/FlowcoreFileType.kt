package org.flowcore.clion

import com.intellij.openapi.fileTypes.LanguageFileType

object FlowcoreFileType : LanguageFileType(FlowcoreLanguage) {
    override fun getName() = "Flowcore"
    override fun getDescription() = "Flowcore source file"
    override fun getDefaultExtension() = "flow"
    override fun getIcon() = null
}
