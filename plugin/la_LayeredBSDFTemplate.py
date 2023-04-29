import maya.mel
from mtoa.ui.ae.shaderTemplate import ShaderAETemplate

class AEla_LayeredBSDFTemplate(ShaderAETemplate):
    def setup(self):
        self.addSwatch()
        self.beginScrollLayout()
        self.addCustom('message', 'AEshaderTypeNew', 'AEshaderTypeReplace')

        self.addControl('thickness', label='Layer Thickness')
        self.addControl('g', label='G')
        self.addControl('albedo', label='Albedo')
        self.addControl('top_normal', label='Top Normal')
        self.addControl('bottom_normal', label='Bottom Normal')

        maya.mel.eval('AEdependNodeTemplate '+self.nodeName)
        self.addExtraControls()
        self.endScrollLayout()
