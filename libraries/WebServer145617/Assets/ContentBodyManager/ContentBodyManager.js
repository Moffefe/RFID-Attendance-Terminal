var ContentBodyManager =
{
	current : null,
	currentDownload : null,
	container : null,
	throbber : null,
	title : null,
	
	init : function(container, throbber, title)
	{
		this.container = container;
		this.throbber = throbber;
		this.title = title;
	},
	
	show : function(name)
	{
		if(this.currentDownload != null)
			return;
		
		let denseName = name.replace(/\s+/g, '');
		
		if(!this.hasOwnProperty(denseName))
		{
			this.download(name);
			return;
		}
		
		if(this.current == this[denseName])
			return;
		
		this.throbber.removeClass('no-visible');
		this.title.html(name);
		
		if(this.current != null)
		{
			if(this.current.hideContentBody != null)
				this.current.hideContentBody();
			
			this.current.addClass('d-none');
		}
		
		this.current = this[denseName];
		this.current.removeClass('d-none');
		
		if(this.current.showContentBody != null)
			this.current.showContentBody();
		
		this.throbber.addClass('no-visible');
	},
	
	download : function(name)
	{
		this.currentDownload = name;
		this.throbber.removeClass('no-visible');
		
		if(this.current != null)
		{
			if(this.current.hideContentBody != null)
				this.current.hideContentBody();
			
			this.current.addClass('d-none');
		}
		
		let urlString = '/';
		urlString += name.replace(/\s+/g, '');
		urlString += "Body.html"
		
		$.ajax(
		{
			url : urlString,
			type : 'GET',
			dataType : 'html',
			success : $.proxy(this.ajaxContentBodyDownloadSuccess, this),
			error : $.proxy(this.ajaxContentBodyDownloadError, this)
		});
	},
	
	ajaxContentBodyDownloadSuccess : function(html)
	{
		let denseName = this.currentDownload.replace(/\s+/g, '');
		this[denseName] = $('<div></div>');
		this[denseName].appendTo(this.container);
		this[denseName].append(html);
	},
	
	ajaxContentBodyDownloadError : function()
	{
		Metro.dialog.create(
		{
			title: 'Error<span class="mif-warning mif-5x fg-red place-right"></span>',
			content: '<div>The requested content cannot be loaded</div>'
		});
		
		if(this.current != null)
		{
			this.current.removeClass('d-none');
			
			if(this.current.showContentBody != null)
				this.current.showContentBody();
		}
		
		this.throbber.addClass('no-visible');
		this.currentDownload = null;
	},
	
	contentBodyReady : function()
	{
		let denseName = this.currentDownload.replace(/\s+/g, '');
		
		let initFunctionName = 'init';
		initFunctionName += denseName;
		
		let showFunctionName = 'show';
		showFunctionName += denseName;
		
		let hideFunctionName = 'hide';
		hideFunctionName += denseName;
		
		let additionalData = {};
		
		if(typeof window[initFunctionName] === 'function')
			additionalData.initContentBody = window[initFunctionName]
		else
			additionalData.initContentBody = null;
		
		if(typeof window[showFunctionName] === 'function')
			additionalData.showContentBody = window[showFunctionName]
		else
			additionalData.showContentBody = null;
		
		if(typeof window[hideFunctionName] === 'function')
			additionalData.hideContentBody = window[hideFunctionName]
		else
			additionalData.hideContentBody = null;
		
		this[denseName] = $.extend({}, this[denseName], additionalData);
		
		if(this[denseName].initContentBody != null)
			this[denseName].initContentBody();
		
		this.title.html(this.currentDownload);
		this.current = this[denseName];
		this.throbber.addClass('no-visible');
		this.currentDownload = null;
	}
}