package org.flowcore.clion

import com.intellij.openapi.fileTypes.PlainSyntaxHighlighter
import com.intellij.openapi.fileTypes.SyntaxHighlighter
import com.intellij.openapi.fileTypes.SyntaxHighlighterFactory

class FlowcoreSyntaxHighlighterFactory : SyntaxHighlighterFactory() {
    override fun getSyntaxHighlighter(project: com.intellij.openapi.project.Project?, virtualFile: com.intellij.openapi.vfs.VirtualFile?): SyntaxHighlighter = PlainSyntaxHighlighter()
}
