# *************************************************************
#  
#  Licensed to the Apache Software Foundation (ASF) under one
#  or more contributor license agreements.  See the NOTICE file
#  distributed with this work for additional information
#  regarding copyright ownership.  The ASF licenses this file
#  to you under the Apache License, Version 2.0 (the
#  "License"); you may not use this file except in compliance
#  with the License.  You may obtain a copy of the License at
#  
#    http://www.apache.org/licenses/LICENSE-2.0
#  
#  Unless required by applicable law or agreed to in writing,
#  software distributed under the License is distributed on an
#  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#  KIND, either express or implied.  See the License for the
#  specific language governing permissions and limitations
#  under the License.
#  
# *************************************************************
all_fragments+=global

# -----------------------------------------------
# count = 12
T4_GLOBAL = \
    writer_Text \
    writer_StarOffice_XML_Writer \
    writer_globaldocument_StarOffice_XML_Writer_GlobalDocument \
    pdf_Portable_Document_Format \
	writerglobal8 \

# -----------------------------------------------
# count = 12
F4_GLOBAL = \
	Text__encoded___StarWriter_GlobalDocument_ \
	writer_globaldocument_StarOffice_XML_Writer \
	writer_globaldocument_StarOffice_XML_Writer_GlobalDocument \
	writer_globaldocument_pdf_Export \
	writerglobal8 \
    writerglobal8_writer \
    writerglobal8_HTML

# -----------------------------------------------
# count = 12
F4_UI_GLOBAL = \
	Text__encoded___StarWriter_GlobalDocument__ui \
	writer_globaldocument_StarOffice_XML_Writer_ui \
	writer_globaldocument_StarOffice_XML_Writer_GlobalDocument_ui \
	writerglobal8_ui \
	writerglobal8_writer_ui

# -----------------------------------------------
# count = 0
L4_GLOBAL =

# -----------------------------------------------
# count = 0
C4_GLOBAL =

# -----------------------------------------------
TYPES_4fcfg_global           = $(foreach,i,$(T4_GLOBAL)    types$/$i.xcu                     )
FILTERS_4fcfg_global         = $(foreach,i,$(F4_GLOBAL)    filters$/$i.xcu                   )
UI_FILTERS_4fcfg_global      = $(foreach,i,$(F4_UI_GLOBAL) $(DIR_LOCFRAG)$/filters$/$i.xcu   )
FRAMELOADERS_4fcfg_global    = $(foreach,i,$(L4_GLOBAL)    frameloaders$/$i.xcu              )
CONTENTHANDLERS_4fcfg_global = $(foreach,i,$(C4_GLOBAL)    contenthandlers$/$i.xcu           )

# -----------------------------------------------
# needed to get dependencies inside global makefile work!
ALL_4fcfg_global = \
    $(TYPES_4fcfg_global) \
    $(FILTERS_4fcfg_global) \
    $(UI_FILTERS_4fcfg_global) \
    $(FRAMELOADERS_4fcfg_global) \
    $(CONTENTHANDLERS_4fcfg_global)
    
ALL_UI_FILTERS+=$(UI_FILTERS_4fcfg_global)
    
ALL_PACKAGES+=global

