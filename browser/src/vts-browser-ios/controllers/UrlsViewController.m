/**
 * Copyright (c) 2017 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#import "UrlsViewController.h"
#import "MapViewController.h"
#import "ConfigViewController.h"

static NSString *itemsStorePath()
{
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    return [[paths objectAtIndex:0] stringByAppendingPathComponent:@"configItems.plist"];
}


@interface UrlsViewController ()
{
    NSMutableArray *urls;
}
@end


@implementation UrlsViewController

- (void)saveItems
{
    [NSKeyedArchiver archiveRootObject:urls toFile:itemsStorePath()];
}

- (void)loadItems
{
    urls = [NSKeyedUnarchiver unarchiveObjectWithFile:itemsStorePath()];
    if (!urls)
    {
        urls = [[NSMutableArray alloc] init];
    }
    if ([urls count] == 0)
    {
        [urls addObject:[[ConfigItem alloc] initWithName:@"Intergeo 2017" url:@"https://cdn.melown.com/mario/store/melown2015/map-config/melown/Melown-Earth-Intergeo-2017/mapConfig.json"]];
    }
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.navigationItem.leftBarButtonItem = self.editButtonItem;
    self.navigationItem.rightBarButtonItems = @[
    	[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemOrganize target:self action:@selector(showOptions)],
    	[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemAdd target:self action:@selector(newItem)]
    ];
    
    [self loadItems];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    for (int i = 0; i < [urls count]; i++)
    {
    	if ([((ConfigItem*)urls[i]).name length] == 0)
    		[urls removeObjectAtIndex:i--];
    }
    [self.tableView reloadData];
    [self saveItems];
}

- (void)newItem
{
    [urls addObject:[[ConfigItem alloc] initWithName:@"" url:@""]];
    NSIndexPath *indexPath = [NSIndexPath indexPathForRow:[urls count] - 1 inSection:0];
    [self.tableView insertRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationBottom];
    [self performSegueWithIdentifier: @"config" sender: [self.tableView cellForRowAtIndexPath:indexPath]];
}

- (void)showOptions
{
	[self performSegueWithIdentifier:@"options" sender:self];
}

#pragma mark - Table view data source

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return [urls count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"UrlCell" forIndexPath:indexPath];
    ConfigItem *item = (ConfigItem*)[urls objectAtIndex:indexPath.row];
    cell.textLabel.text = item.name;
    return cell;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (editingStyle == UITableViewCellEditingStyleDelete)
    {
        [urls removeObjectAtIndex:indexPath.row];
        [tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
        [self saveItems];
    }
}

- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)sourceIndexPath toIndexPath:(NSIndexPath *)destinationIndexPath
{
    NSObject *tmp = [urls objectAtIndex:sourceIndexPath.row];
    [urls removeObjectAtIndex:sourceIndexPath.row];
    [urls insertObject:tmp atIndex:destinationIndexPath.row];
    [self saveItems];
}

#pragma mark - Navigation

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    NSIndexPath *indexPath = [self.tableView indexPathForCell:sender];
    ConfigItem *item = (ConfigItem*)[urls objectAtIndex:indexPath.row];
    if ([[segue identifier] isEqualToString:@"map"])
    {
        MapViewController *controller = (MapViewController *)[segue destinationViewController];
        [controller setItem:item];
    }
    if ([[segue identifier] isEqualToString:@"config"])
    {
        ConfigViewController *controller = (ConfigViewController *)[segue destinationViewController];
        [controller setItem:item];
    }
}

@end
