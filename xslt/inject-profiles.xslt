<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:param name="profile-dir"/>
  <xsl:param name="img-height">30%</xsl:param>
  <xsl:param name="img-width">30%</xsl:param>

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="div[@class='diveProfile' and starts-with(@id,'dive_')]">
    <img>
      <xsl:attribute name="height"><xsl:value-of select="$img-height"/></xsl:attribute>
      <xsl:attribute name="width"><xsl:value-of select="$img-width"/></xsl:attribute>
      <xsl:attribute name="src">
        <xsl:value-of select="concat('file:///', $profile-dir, '/dive_', substring-after(@id,'dive_'), '.png')"/>
      </xsl:attribute>
    </img>
  </xsl:template>
</xsl:stylesheet>
