import maya.mel
from mtoa.ui.ae.shaderTemplate import ShaderAETemplate

class AEla_DielectricBSDFTemplate(ShaderAETemplate):
    def setup(self):
        self.addSwatch()
        self.beginScrollLayout()
        self.addCustom('message', 'AEshaderTypeNew', 'AEshaderTypeReplace')

        self.addControl('ior', label='Index of Refraction')
        self.addControl('roughness', label='Roughness')

        maya.mel.eval('AEdependNodeTemplate '+self.nodeName)
        self.addExtraControls()
        self.endScrollLayout()
